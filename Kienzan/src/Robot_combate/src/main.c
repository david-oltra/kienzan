#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "nvs_flash.h"

/* =========================================================
 *  ESC (motor brushless) - GPIO4
 * ========================================================= */
#define ESC_GPIO           4
#define ESC_TIMER          LEDC_TIMER_0
#define ESC_CHANNEL        LEDC_CHANNEL_0
#define ESC_DUTY_RES       LEDC_TIMER_13_BIT
#define ESC_FREQUENCY      50
#define ESC_DUTY_MAX       ((1 << 13) - 1)

#define ESC_PULSE_MIN_US   1000
#define ESC_PULSE_MAX_US   2000

/* Umbral mínimo para que el motor brushless arranque.
 * Por debajo de este %, el ESC se queda en mínimo (parado). */
#define ESC_MIN_START_PERCENT  30

/* =========================================================
 *  TB6612 - 2 motores DC
 * ========================================================= */
#define PWMA_GPIO   5
#define AIN2_GPIO   6
#define AIN1_GPIO   7
#define STBY_GPIO   8
#define BIN1_GPIO   9
#define BIN2_GPIO   10
#define PWMB_GPIO   20
#define GND_GPIO    21   /* ⚠️ Mejor conéctalo a GND real */

/* Invertir sentido de giro de cada motor.
 * Pon 1 en el motor que gire al revés de lo esperado. */
#define MOTOR_A_INVERT  1
#define MOTOR_B_INVERT  1

#define MOTOR_TIMER        LEDC_TIMER_1
#define MOTOR_DUTY_RES     LEDC_TIMER_10_BIT
#define MOTOR_FREQUENCY    20000
#define MOTOR_DUTY_MAX     ((1 << 10) - 1)

#define CH_PWMA            LEDC_CHANNEL_1
#define CH_PWMB            LEDC_CHANNEL_2

/* =========================================================
 *  ESP-NOW
 * ========================================================= */
#define ESPNOW_CHANNEL     1
#define REMOTE_TIMEOUT_MS  500

/* MAC del mando emisor */
static const uint8_t REMOTE_MAC[6] = {0xC8, 0x2E, 0x18, 0xF1, 0x83, 0x40};

typedef struct __attribute__((packed)) {
    int8_t  joystick_right_Y;   /* -100..+100 */
    int8_t  joystick_right_X;   /* -100..+100 */
    uint8_t joystick_left_Y;    /*    0..100  */
    int8_t  joystick_left_X;    /* -100..+100 */
} remote_data_t;

/* =========================================================
 *  Tipos y estado global
 * ========================================================= */
typedef enum {
    MOTOR_A = 0,
    MOTOR_B = 1
} motor_id_t;

static QueueHandle_t s_remote_queue = NULL;

/* =========================================================
 *  Prototipos
 * ========================================================= */
static void esc_init(void);
static void esc_set_pulse_width(uint32_t pulse_us);
static void esc_set_speed_percent(uint8_t percent);

static void motors_init(void);
static void motor_set(motor_id_t motor, int8_t speed_percent);
static void motors_stop_all(void);

static void wifi_init_sta(void);
static void espnow_init(void);
static void on_data_recv(const esp_now_recv_info_t *info,
                         const uint8_t *data, int len);

static void apply_remote(const remote_data_t *r);

/* =========================================================
 *  app_main
 * ========================================================= */
void app_main(void)
{
    printf("\n=== Receptor ESP-NOW: ESC + TB6612 ===\n");

    /* 1. Motores DC */
    motors_init();

    /* 2. ESC + secuencia de armado */
    esc_init();
    printf("Armando ESC (3s)...\n");
    esc_set_pulse_width(ESC_PULSE_MIN_US);
    vTaskDelay(pdMS_TO_TICKS(3000));
    printf("ESC armado.\n");

    /* 3. WiFi + ESP-NOW */
    wifi_init_sta();
    espnow_init();

    printf("Esperando datos del mando...\n");

    /* 4. Bucle principal */
    remote_data_t r;
    TickType_t last_rx = xTaskGetTickCount();
    bool link_ok = false;

    while (1) {
        if (xQueueReceive(s_remote_queue, &r, pdMS_TO_TICKS(50)) == pdTRUE) {
            last_rx = xTaskGetTickCount();
            if (!link_ok) {
                printf("Link establecido.\n");
                link_ok = true;
            }
            apply_remote(&r);
        }

        /* Failsafe: sin datos -> parar todo */
        if (link_ok &&
            (xTaskGetTickCount() - last_rx) > pdMS_TO_TICKS(REMOTE_TIMEOUT_MS)) {
            printf("Timeout de link, parando motores.\n");
            motors_stop_all();
            esc_set_speed_percent(0);
            link_ok = false;
        }
    }
}

/* =========================================================
 *  ESC
 * ========================================================= */
static void esc_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = ESC_TIMER,
        .duty_resolution = ESC_DUTY_RES,
        .freq_hz         = ESC_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = ESC_CHANNEL,
        .timer_sel  = ESC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = ESC_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    printf("ESC PWM: %d Hz, %d bits, GPIO%d (min arranque %d%%)\n",
           ESC_FREQUENCY, ESC_DUTY_RES, ESC_GPIO, ESC_MIN_START_PERCENT);
}

static void esc_set_pulse_width(uint32_t pulse_us)
{
    if (pulse_us < ESC_PULSE_MIN_US) pulse_us = ESC_PULSE_MIN_US;
    if (pulse_us > ESC_PULSE_MAX_US) pulse_us = ESC_PULSE_MAX_US;

    uint32_t duty = (pulse_us * ESC_DUTY_MAX) / 20000;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, ESC_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, ESC_CHANNEL));
}

/**
 * @brief Aplica un porcentaje de velocidad al ESC.
 *
 * Si 'percent' es menor que ESC_MIN_START_PERCENT, el ESC se queda
 * en mínimo (motor parado). Una vez superado el umbral, el porcentaje
 * se mapea linealmente desde ESC_MIN_START_PERCENT..100 hasta
 * ESC_PULSE_MIN_US..ESC_PULSE_MAX_US.
 */
static void esc_set_speed_percent(uint8_t percent)
{
    if (percent > 100) percent = 100;

    /* Por debajo del umbral -> mínimo (parado) */
    if (percent < ESC_MIN_START_PERCENT) {
        esc_set_pulse_width(ESC_PULSE_MIN_US);
        return;
    }

    /* Re-mapear [ESC_MIN_START_PERCENT..100] -> [0..100] y luego a pulso */
    uint32_t mapped = ((uint32_t)(percent - ESC_MIN_START_PERCENT) * 100) /
                      (100 - ESC_MIN_START_PERCENT);

    uint32_t pulse_us = ESC_PULSE_MIN_US +
        ((ESC_PULSE_MAX_US - ESC_PULSE_MIN_US) * mapped) / 100;

    esc_set_pulse_width(pulse_us);
}

/* =========================================================
 *  TB6612
 * ========================================================= */
static void motors_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << AIN1_GPIO) |
                        (1ULL << AIN2_GPIO) |
                        (1ULL << BIN1_GPIO) |
                        (1ULL << BIN2_GPIO) |
                        (1ULL << STBY_GPIO) |
                        (1ULL << GND_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    gpio_set_level(GND_GPIO, 0);   /* ⚠️ idealmente GND real */

    gpio_set_level(AIN1_GPIO, 0);
    gpio_set_level(AIN2_GPIO, 0);
    gpio_set_level(BIN1_GPIO, 0);
    gpio_set_level(BIN2_GPIO, 0);

    ledc_timer_config_t motor_timer = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = MOTOR_TIMER,
        .duty_resolution = MOTOR_DUTY_RES,
        .freq_hz         = MOTOR_FREQUENCY,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&motor_timer));

    ledc_channel_config_t ch_a = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = CH_PWMA,
        .timer_sel  = MOTOR_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = PWMA_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_a));

    ledc_channel_config_t ch_b = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = CH_PWMB,
        .timer_sel  = MOTOR_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = PWMB_GPIO,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_b));

    gpio_set_level(STBY_GPIO, 1);   /* Activar driver */

    printf("TB6612 listo: PWMA=GPIO%d, PWMB=GPIO%d, STBY=GPIO%d, %d Hz\n",
           PWMA_GPIO, PWMB_GPIO, STBY_GPIO, MOTOR_FREQUENCY);
}

static void motor_set(motor_id_t motor, int8_t speed_percent)
{
    if (speed_percent >  100) speed_percent =  100;
    if (speed_percent < -100) speed_percent = -100;

    /* Invertir sentido si el motor está montado al revés */
    if (motor == MOTOR_A && MOTOR_A_INVERT) {
        speed_percent = -speed_percent;
    }
    if (motor == MOTOR_B && MOTOR_B_INVERT) {
        speed_percent = -speed_percent;
    }

    uint32_t duty = (abs(speed_percent) * MOTOR_DUTY_MAX) / 100;

    ledc_channel_t ch;
    int gpio_in1, gpio_in2;

    if (motor == MOTOR_A) {
        ch       = CH_PWMA;
        gpio_in1 = AIN1_GPIO;
        gpio_in2 = AIN2_GPIO;
    } else {
        ch       = CH_PWMB;
        gpio_in1 = BIN1_GPIO;
        gpio_in2 = BIN2_GPIO;
    }

    if (speed_percent > 0) {
        gpio_set_level(gpio_in1, 1);
        gpio_set_level(gpio_in2, 0);
    } else if (speed_percent < 0) {
        gpio_set_level(gpio_in1, 0);
        gpio_set_level(gpio_in2, 1);
    } else {
        gpio_set_level(gpio_in1, 0);
        gpio_set_level(gpio_in2, 0);   /* coast */
    }

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, ch, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, ch));
}

static void motors_stop_all(void)
{
    motor_set(MOTOR_A, 0);
    motor_set(MOTOR_B, 0);
}

/* =========================================================
 *  WiFi + ESP-NOW
 * ========================================================= */
static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Canal fijo: el emisor debe usar el mismo */
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
    printf("MAC STA: %02X:%02X:%02X:%02X:%02X:%02X (canal %d)\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
           ESPNOW_CHANNEL);
}

static void on_data_recv(const esp_now_recv_info_t *info,
                         const uint8_t *data, int len)
{
    /* Filtrar por MAC de origen */
    if (memcmp(info->src_addr, REMOTE_MAC, 6) != 0) {
        return;
    }

    if (len != (int)sizeof(remote_data_t)) {
        printf("Paquete ignorado (len=%d, esperado=%d)\n",
               len, (int)sizeof(remote_data_t));
        return;
    }

    xQueueOverwrite(s_remote_queue, data);
}

static void espnow_init(void)
{
    s_remote_queue = xQueueCreate(1, sizeof(remote_data_t));
    configASSERT(s_remote_queue != NULL);

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));

    /* Peer = MAC concreta del mando */
    if (!esp_now_is_peer_exist(REMOTE_MAC)) {
        esp_now_peer_info_t peer = {
            .channel = ESPNOW_CHANNEL,
            .encrypt = false,
        };
        memcpy(peer.peer_addr, REMOTE_MAC, 6);
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    }

    printf("Peer registrado: %02X:%02X:%02X:%02X:%02X:%02X\n",
           REMOTE_MAC[0], REMOTE_MAC[1], REMOTE_MAC[2],
           REMOTE_MAC[3], REMOTE_MAC[4], REMOTE_MAC[5]);
}

/* =========================================================
 *  Aplicar datos del mando a los motores
 * ========================================================= */
static void apply_remote(const remote_data_t *r)
{
    /* --- Motores DC: joystick derecho --- */
    int x = r->joystick_right_X;
    int y = r->joystick_right_Y;

    if (x >  100) x =  100;
    if (x < -100) x = -100;
    if (y >  100) y =  100;
    if (y < -100) y = -100;

    /* Mezcla tipo RC (tank mixing):
     *   - Centro (X=0): ambos motores a Y (hasta 100% si Y=100).
     *   - X positivo: reparte velocidad hacia un lado para girar.
     *
     *   left  = y + x
     *   right = y - x
     *
     * Luego se satura a [-100, 100]. */
    int left_speed  = y + x;
    int right_speed = y - x;

    if (left_speed  >  100) left_speed  =  100;
    if (left_speed  < -100) left_speed  = -100;
    if (right_speed >  100) right_speed =  100;
    if (right_speed < -100) right_speed = -100;

    motor_set(MOTOR_A, (int8_t)left_speed);
    motor_set(MOTOR_B, (int8_t)right_speed);

    /* --- Motor brushless: joystick izquierdo Y --- */
    uint8_t esc_pct = r->joystick_left_Y;
    if (esc_pct > 100) esc_pct = 100;
    esc_set_speed_percent(esc_pct);
}