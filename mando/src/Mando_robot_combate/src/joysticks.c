/* joysticks.c */

#include "joysticks.h"
#include "app_config.h"
#include "nvs_manager.h"
#include "remote_types.h"

#include <string.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "JOY";

/* ---------- Inversión de ejes ---------- */
#define INVERT_RIGHT_Y  1
#define INVERT_RIGHT_X  0
#define INVERT_LEFT_Y   1
#define INVERT_LEFT_X   0

/* Umbral para considerar el joystick izquierdo Y "a cero".
 * El eje ya viene escalado 0..100, así que 3 da margen al ruido. */
#define ARM_THRESHOLD   3

/* ---------- Nombres de claves NVS para cada eje ---------- */
static const char *s_keys[4][2] = {
    { "RVmin", "RVmax" },
    { "RHmin", "RHmax" },
    { "LVmin", "LVmax" },
    { "LHmin", "LHmax" },
};

static const adc_channel_t s_channels[4] = {
    JOY_RV_CH, JOY_RH_CH, JOY_LV_CH, JOY_LH_CH,
};

/* ---------- Estado interno ---------- */

static adc_oneshot_unit_handle_t s_adc = NULL;
static int16_t  s_cal[4][2];
static volatile bool s_calibrating = false;
static volatile bool s_armed       = false;

/* ---------- Helpers ADC ---------- */

static esp_err_t adc_setup(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = JOY_ADC_UNIT,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc);
    if (err != ESP_OK) return err;

    adc_oneshot_chan_cfg_t ch_cfg = {
        .bitwidth = JOY_ADC_BITWIDTH,
        .atten    = JOY_ADC_ATTEN,
    };
    for (int i = 0; i < 4; i++) {
        err = adc_oneshot_config_channel(s_adc, s_channels[i], &ch_cfg);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

static inline int adc_read_single(adc_channel_t ch)
{
    int raw = 0;
    adc_oneshot_read(s_adc, ch, &raw);
    return raw;
}

static int adc_read_avg(adc_channel_t ch, int n)
{
    int32_t acc = 0;
    int raw = 0;
    for (int i = 0; i < n; i++) {
        adc_oneshot_read(s_adc, ch, &raw);
        acc += raw;
    }
    return (int)(acc / n);
}

/* ---------- Carga de calibración desde NVS ---------- */

static void load_calibration(void)
{
    for (int i = 0; i < 4; i++) {
        int16_t lo = 0, hi = 0;
        esp_err_t e1 = nvs_manager_get_i16(s_keys[i][0], &lo);
        esp_err_t e2 = nvs_manager_get_i16(s_keys[i][1], &hi);

        if (e1 == ESP_ERR_NVS_NOT_FOUND || e2 == ESP_ERR_NVS_NOT_FOUND) {
            lo = 0; hi = 4095;
        } else if (e1 != ESP_OK || e2 != ESP_OK) {
            lo = 0; hi = 4095;
        } else if (hi <= lo) {
            lo = 0; hi = 4095;
        }
        s_cal[i][0] = lo;
        s_cal[i][1] = hi;
    }
}

/* ---------- Escalado ---------- */

static inline int8_t scale_axis_bipolar(int raw, int idx)
{
    int lo = s_cal[idx][0];
    int hi = s_cal[idx][1];
    if (hi <= lo) hi = lo + 1;

    const int center = (lo + hi) / 2;
    int32_t v;

    if (raw >= center) {
        int span = hi - center;
        if (span < 1) span = 1;
        v =  ((int32_t)(raw - center) * 100) / span;
    } else {
        int span = center - lo;
        if (span < 1) span = 1;
        v = -((int32_t)(center - raw) * 100) / span;
    }

    if (v < JOY_BIP_MIN) v = JOY_BIP_MIN;
    if (v > JOY_BIP_MAX) v = JOY_BIP_MAX;
    return (int8_t)v;
}

static inline uint8_t scale_axis_unipolar(int raw, int idx)
{
    int lo = s_cal[idx][0];
    int hi = s_cal[idx][1];
    if (hi <= lo) hi = lo + 1;

    int32_t v = ((int32_t)(raw - lo) * 100) / (hi - lo);
    if (v < JOY_UNI_MIN) v = JOY_UNI_MIN;
    if (v > JOY_UNI_MAX) v = JOY_UNI_MAX;
    return (uint8_t)v;
}

static inline int8_t apply_deadzone(int8_t v, int8_t dz)
{
    return (v > -dz && v < dz) ? 0 : v;
}

#define JOY_DEADZONE 3

/* ---------- Init ---------- */

esp_err_t joysticks_init(void)
{
    esp_err_t err = adc_setup();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_setup failed: %s", esp_err_to_name(err));
        return err;
    }
    load_calibration();
    return ESP_OK;
}

/* ---------- Tarea de lectura continua ---------- */

void joysticks_task(void *arg)
{
    (void)arg;

    const int N = 200;

    while (1) {
        if (s_calibrating) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        int32_t acc[4] = {0};
        for (int s = 0; s < N; s++) {
            for (int i = 0; i < 4; i++) {
                acc[i] += adc_read_single(s_channels[i]);
            }
        }

        int rv = acc[0] / N;
        int rh = acc[1] / N;
        int lv = acc[2] / N;
        int lh = acc[3] / N;

        int8_t ry_val = scale_axis_bipolar(rv, 0);
#if INVERT_RIGHT_Y
        ry_val = -ry_val;
#endif
        remote_data.joystick_right_Y = apply_deadzone(ry_val, JOY_DEADZONE);

        int8_t rx_val = scale_axis_bipolar(rh, 1);
#if INVERT_RIGHT_X
        rx_val = -rx_val;
#endif
        remote_data.joystick_right_X = apply_deadzone(rx_val, JOY_DEADZONE);

        uint8_t ly_val = scale_axis_unipolar(lv, 2);
#if INVERT_LEFT_Y
        ly_val = (uint8_t)(100 - ly_val);
#endif
        remote_data.joystick_left_Y = ly_val;

        int8_t lx_val = scale_axis_bipolar(lh, 3);
#if INVERT_LEFT_X
        lx_val = -lx_val;
#endif
        remote_data.joystick_left_X = apply_deadzone(lx_val, JOY_DEADZONE);

        /* --- Comprobación de armado --- */
        if (!s_armed && remote_data.joystick_left_Y <= ARM_THRESHOLD) {
            s_armed = true;
            ESP_LOGI(TAG, "system armed");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ---------- Calibración ---------- */

bool joysticks_is_calibrating(void)
{
    return s_calibrating;
}

bool joysticks_is_armed(void)
{
    return s_armed;
}

static int wait_and_sample(adc_channel_t ch, bool wait_high, joy_progress_cb_t cb, const char *msg)
{
    if (cb) cb(msg);

    while (1) {
        int v = adc_read_avg(ch, 500);
        if (wait_high ? (v > JOY_CAL_LOW_DETECT) : (v < JOY_CAL_HIGH_DETECT)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return adc_read_avg(ch, 10000);
}

static void calibrate_axis(int idx,
                           adc_channel_t ch,
                           const char *msg_a,
                           const char *msg_b,
                           joy_progress_cb_t cb)
{
    if (cb) cb(msg_a);
    while (1) {
        int v = adc_read_avg(ch, 500);
        if (v > JOY_CAL_LOW_DETECT || v < JOY_CAL_HIGH_DETECT) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    int v_a = adc_read_avg(ch, 10000);

    if (cb) cb(msg_b);
    bool wait_high = (v_a < 2048);
    while (1) {
        int v = adc_read_avg(ch, 500);
        if (wait_high ? (v > JOY_CAL_LOW_DETECT) : (v < JOY_CAL_HIGH_DETECT)) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    int v_b = adc_read_avg(ch, 10000);

    int v_lo = (v_a < v_b) ? v_a : v_b;
    int v_hi = (v_a > v_b) ? v_a : v_b;

    nvs_manager_set_i16(s_keys[idx][0], (int16_t)v_lo);
    nvs_manager_set_i16(s_keys[idx][1], (int16_t)v_hi);
}

void joysticks_calibrate(joy_progress_cb_t cb)
{
    if (s_calibrating) {
        return;
    }
    s_calibrating = true;

    calibrate_axis(0, JOY_RV_CH,
                   "Mover joystick\nderecho a un tope",
                   "Mover joystick\nderecho al tope opuesto",
                   cb);

    calibrate_axis(1, JOY_RH_CH,
                   "Mover joystick\nderecho a un tope",
                   "Mover joystick\nderecho al tope opuesto",
                   cb);

    calibrate_axis(2, JOY_LV_CH,
                   "Mover joystick\nizquierdo a un tope",
                   "Mover joystick\nizquierdo al tope opuesto",
                   cb);

    calibrate_axis(3, JOY_LH_CH,
                   "Mover joystick\nizquierdo a un tope",
                   "Mover joystick\nizquierdo al tope opuesto",
                   cb);

    load_calibration();

    s_calibrating = false;
}