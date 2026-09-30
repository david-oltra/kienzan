/* ui.c */

#include "ui.h"
#include "app_config.h"
#include "buttons.h"
#include "joysticks.h"
#include "comm.h"
#include "nvs_manager.h"
#include "remote_types.h"
#include "displays.h"

#include <string.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <lvgl.h>
#include <lvgl_helpers.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char *TAG = "UI";

/* ---------- Configuración ---------- */
#define UI_UPDATE_PERIOD_MS  100
#define UI_ANIM_MS           500
#define SPLASH_DURATION_MS   2500
#define SPLASH_FADE_MS       500

/* ---------- Modos de la UI ---------- */
typedef enum {
    UI_MODE_SPLASH,
    UI_MODE_WARNING,
    UI_MODE_NORMAL,
} ui_mode_t;

/* ---------- Estado interno ---------- */

static SemaphoreHandle_t s_lvgl_mutex = NULL;

static screen_id_t s_screen      = SCR_HOME;
static screen_id_t s_prev_screen = SCR_HOME;
static int8_t      s_menu_idx    = 0;
static int8_t      s_menu_prev   = 0;

static uint8_t s_enabled[SCR_COUNT] = {1,1,1,1,1,1,1};

static TickType_t s_splash_until_tick = 0;
static ui_mode_t  s_ui_mode           = UI_MODE_SPLASH;

static const char *s_screen_keys[SCR_COUNT] = {
    "home", "home_chart", "panel", "motor", "bateria", "gps", "reloj"
};

/* ---------- Items del menú de configuración ---------- */

typedef enum {
    CFG_NONE = 0,
    CFG_CALIBRATE,
    CFG_TOGGLE,
    CFG_EXIT,
} cfg_action_t;

typedef struct {
    int8_t       menu_idx;
    cfg_action_t action;
    int8_t       screen;
} cfg_item_t;

static const cfg_item_t s_cfg_items[] = {
    { 2,  CFG_CALIBRATE, -1 },
    { 4,  CFG_TOGGLE,    SCR_HOME       },
    { 5,  CFG_TOGGLE,    SCR_HOME_CHART },
    { 6,  CFG_TOGGLE,    SCR_PANEL      },
    { 7,  CFG_TOGGLE,    SCR_MOTOR      },
    { 8,  CFG_TOGGLE,    SCR_BATERIA    },
    { 9,  CFG_TOGGLE,    SCR_GPS        },
    { 10, CFG_TOGGLE,    SCR_RELOJ      },
    { 12, CFG_EXIT,      -1 },
};
#define N_CFG_ITEMS (sizeof(s_cfg_items) / sizeof(s_cfg_items[0]))

static const cfg_item_t *cfg_find(int8_t menu_idx)
{
    for (size_t i = 0; i < N_CFG_ITEMS; i++) {
        if (s_cfg_items[i].menu_idx == menu_idx) return &s_cfg_items[i];
    }
    return NULL;
}

static int8_t cfg_next(int8_t current, int direction)
{
    int idx = current;
    for (int i = 0; i < 20; i++) {
        idx += direction;
        if (idx < 2 || idx > 12) return current;
        if (cfg_find(idx)) return (int8_t)idx;
    }
    return current;
}

/* ---------- Helpers LVGL con mutex ---------- */

static inline void lvgl_lock(void)   { xSemaphoreTake(s_lvgl_mutex, portMAX_DELAY); }
static inline void lvgl_unlock(void) { xSemaphoreGive(s_lvgl_mutex); }

/* ---------- Mapeo screen_id_t → lv_obj_t* ---------- */

static lv_obj_t *screen_obj(screen_id_t scr)
{
    switch (scr) {
        case SCR_HOME:       return home;
        case SCR_HOME_CHART: return home_chart;
        case SCR_PANEL:      return screen_panel;
        case SCR_MOTOR:      return screen_motor;
        case SCR_BATERIA:    return screen_bateria;
        case SCR_GPS:        return screen_gps;
        case SCR_RELOJ:      return screen_reloj;
        case SCR_MENU:       return screen_menu;
        default:             return NULL;
    }
}

static screen_id_t next_enabled(screen_id_t from, int direction)
{
    const int n = SCR_MENU + 1;
    int idx = (int)from;
    for (int i = 0; i < n; i++) {
        idx += direction;
        if (idx >= n) idx = 0;
        if (idx < 0)  idx = n - 1;
        if (idx == SCR_MENU) return SCR_MENU;
        if (s_enabled[idx])  return (screen_id_t)idx;
    }
    return from;
}

/* ---------- Init LVGL ---------- */

static void lvgl_setup(void)
{
    lv_init();
    lvgl_driver_init();

    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf_1[240 * 40];
    static lv_color_t buf_2[240 * 40];
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, 240 * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = st7789_flush;
    disp_drv.hor_res  = 280;
    disp_drv.ver_res  = 240;
    lv_disp_drv_register(&disp_drv);
}

static void refresh_menu_opacities_locked(void);

esp_err_t ui_init(void)
{
    s_lvgl_mutex = xSemaphoreCreateMutex();
    if (!s_lvgl_mutex) return ESP_ERR_NO_MEM;

    lvgl_setup();

    for (int i = 0; i < SCR_COUNT; i++) {
        uint8_t v = 0;
        esp_err_t err = nvs_manager_get_u8(s_screen_keys[i], &v);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            v = 1;
            nvs_manager_set_u8(s_screen_keys[i], 1);
        }
        s_enabled[i] = v;
    }

    lvgl_lock();
    ui_build_screens();
    refresh_menu_opacities_locked();

    /* Solo cargamos splash. La decisión de ir a HOME o WARNING la toma
     * ui_task cuando termina el splash, según joysticks_is_armed(). */
    lv_scr_load(screen_splash);
    lvgl_unlock();

    s_splash_until_tick = xTaskGetTickCount() +
                          pdMS_TO_TICKS(SPLASH_DURATION_MS);

    return ESP_OK;
}

/* ---------- Helpers de pantalla ---------- */

static void refresh_menu_opacities_locked(void)
{
    for (int i = 0; i < SCR_COUNT; i++) {
        lv_obj_t *btn = lv_obj_get_child(list_menu_config, i + 3);
        lv_obj_set_style_opa(btn, s_enabled[i] ? LV_OPA_100 : LV_OPA_50, 0);
    }
}

static void menu_highlight_locked(int8_t idx, int8_t prev)
{
    lv_obj_t *btn = lv_obj_get_child(list_menu_config, idx - 1);
    lv_obj_add_state(btn, LV_STATE_CHECKED);
    lv_obj_scroll_to_view(btn, LV_ANIM_ON);
    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_30, 0);

    if (prev > 0) {
        lv_obj_t *pbtn = lv_obj_get_child(list_menu_config, prev - 1);
        lv_obj_set_style_bg_color(pbtn, lv_color_black(), 0);
    }
}

static void load_screen(screen_id_t scr, bool from_right)
{
    lv_obj_t *target = screen_obj(scr);
    if (!target) return;

    lv_scr_load_anim_t anim = from_right
        ? LV_SCR_LOAD_ANIM_MOVE_RIGHT
        : LV_SCR_LOAD_ANIM_MOVE_LEFT;

    lvgl_lock();
    lv_scr_load_anim(target, anim, UI_ANIM_MS, 0, false);
    lvgl_unlock();
}

/* ---------- Navegación de pantallas ---------- */

static void nav_up(void)
{
    if (s_menu_idx == 0) {
        screen_id_t next = next_enabled(s_screen, +1);
        if (next != s_screen) {
            s_prev_screen = s_screen;
            s_screen = next;
            load_screen(s_screen, false);
        }
    } else {
        int8_t next = cfg_next(s_menu_idx, -1);
        if (next != s_menu_idx) {
            s_menu_prev = s_menu_idx;
            s_menu_idx = next;
            lvgl_lock();
            menu_highlight_locked(s_menu_idx, s_menu_prev);
            lvgl_unlock();
        }
    }
}

static void nav_down(void)
{
    if (s_menu_idx == 0) {
        screen_id_t next = next_enabled(s_screen, -1);
        if (next != s_screen) {
            s_prev_screen = s_screen;
            s_screen = next;
            load_screen(s_screen, true);
        }
    } else {
        int8_t next = cfg_next(s_menu_idx, +1);
        if (next != s_menu_idx) {
            s_menu_prev = s_menu_idx;
            s_menu_idx = next;
            lvgl_lock();
            menu_highlight_locked(s_menu_idx, s_menu_prev);
            lvgl_unlock();
        }
    }
}

/* ---------- Acciones del menú de configuración ---------- */

static void calibration_progress_cb(const char *msg)
{
    lvgl_lock();
    lv_label_set_text(label_calibrate_center, msg);
    lvgl_unlock();
}

static void action_calibrate(void)
{
    lvgl_lock();
    lv_scr_load(screen_calibrate);
    lvgl_unlock();

    joysticks_calibrate(calibration_progress_cb);

    xQueueReset(buttons_get_queue());

    lvgl_lock();
    lv_scr_load(screen_menu_config);
    menu_highlight_locked(s_menu_idx, 0);
    lvgl_unlock();
}

static void action_toggle(int screen_idx)
{
    s_enabled[screen_idx] = !s_enabled[screen_idx];
    nvs_manager_set_u8(s_screen_keys[screen_idx], s_enabled[screen_idx]);

    lvgl_lock();
    lv_obj_t *btn = lv_obj_get_child(list_menu_config, screen_idx + 3);
    lv_obj_set_style_opa(btn, s_enabled[screen_idx] ? LV_OPA_100 : LV_OPA_50, 0);
    lvgl_unlock();
}

static void action_exit(void)
{
    s_menu_idx = 0;
    s_menu_prev = 0;
    s_prev_screen = SCR_MENU;
    s_screen = SCR_HOME;
    load_screen(SCR_HOME, true);
}

static void nav_ok(void)
{
    if (s_screen == SCR_MENU && s_menu_idx == 0) {
        s_menu_idx = 2;
        s_menu_prev = 0;
        lvgl_lock();
        refresh_menu_opacities_locked();
        lv_scr_load(screen_menu_config);
        menu_highlight_locked(s_menu_idx, 0);
        lvgl_unlock();
        return;
    }

    if (s_menu_idx != 0) {
        const cfg_item_t *item = cfg_find(s_menu_idx);
        if (!item) return;

        switch (item->action) {
            case CFG_CALIBRATE: action_calibrate(); break;
            case CFG_TOGGLE:    action_toggle(item->screen); break;
            case CFG_EXIT:      action_exit(); break;
            default: break;
        }
    }
}

/* ---------- Actualización de datos de pantalla ---------- */

static void update_current_screen(void)
{
    received_data_t rx;
    if (comm_get_received(&rx) != ESP_OK) return;

    float p_power = (rx.panel_volt  / 1000.0f) * (rx.panel_amp  / 1000.0f);
    float m_power = (rx.motor1_volt / 1000.0f) * (rx.motor1_amp / 1000.0f);

    lvgl_lock();
    switch (s_screen) {
        case SCR_HOME:
            change_home(remote_data.joystick_left_Y,
                        remote_data.joystick_right_Y);
            break;
        case SCR_HOME_CHART:
            change_home_chart((uint8_t)m_power, (uint8_t)p_power);
            break;
        case SCR_PANEL:
            change_screen_panel(rx.panel_volt / 1000.0f,
                                rx.panel_amp  / 1000.0f);
            break;
        case SCR_MOTOR:
            change_screen_motor(rx.motor1_volt / 1000.0f,
                                rx.motor1_amp  / 1000.0f);
            break;
        case SCR_BATERIA:
            change_screen_bateria(4.2f, rx.battery);
            break;
        case SCR_GPS:
            change_screen_gps(rx.gps_latitude, rx.gps_longitude);
            break;
        case SCR_RELOJ:
            change_screen_reloj(rx.gps_hour, rx.gps_minute,
                                rx.gps_year, rx.gps_month, rx.gps_day);
            break;
        default:
            break;
    }
    lvgl_unlock();
}

/* ---------- Tareas ---------- */

void ui_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "ui task started");

    QueueHandle_t btn_q = buttons_get_queue();
    button_event_t ev;
    TickType_t last_update = 0;

    while (1) {
        /* --- Fase SPLASH --- */
        if (s_ui_mode == UI_MODE_SPLASH) {
            if (xTaskGetTickCount() < s_splash_until_tick) {
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }
            /* Splash terminado: decidir siguiente pantalla. */
            xQueueReset(btn_q);
            lvgl_lock();
            if (joysticks_is_armed()) {
                lv_scr_load_anim(home, LV_SCR_LOAD_ANIM_FADE_ON,
                                 SPLASH_FADE_MS, 0, false);
                s_ui_mode = UI_MODE_NORMAL;
                s_screen = SCR_HOME;
                s_prev_screen = SCR_HOME;
            } else {
                lv_scr_load(screen_warning);
                s_ui_mode = UI_MODE_WARNING;
            }
            lvgl_unlock();
            last_update = 0;
            continue;
        }

        /* --- Fase WARNING --- */
        if (s_ui_mode == UI_MODE_WARNING) {
            xQueueReset(btn_q);   /* descartar pulsaciones acumuladas */
            if (joysticks_is_armed()) {
                lvgl_lock();
                lv_scr_load_anim(home, LV_SCR_LOAD_ANIM_FADE_ON,
                                 SPLASH_FADE_MS, 0, false);
                lvgl_unlock();
                s_ui_mode = UI_MODE_NORMAL;
                s_screen = SCR_HOME;
                s_prev_screen = SCR_HOME;
                last_update = 0;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* --- Fase NORMAL --- */
        if (xQueueReceive(btn_q, &ev, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (!joysticks_is_calibrating()) {
                switch (ev.id) {
                    case BTN_UP:   nav_up();   break;
                    case BTN_DOWN: nav_down(); break;
                    case BTN_OK:   nav_ok();   break;
                    default: break;
                }
            }
            last_update = 0;
        }

        TickType_t now = xTaskGetTickCount();
        if (now - last_update >= pdMS_TO_TICKS(UI_UPDATE_PERIOD_MS)) {
            update_current_screen();
            last_update = now;
        }
    }
}

void ui_lvgl_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "lvgl task started");

    int64_t last = esp_timer_get_time();

    while (1) {
        int64_t now = esp_timer_get_time();
        uint32_t elapsed_ms = (uint32_t)((now - last) / 1000);
        last = now;

        lvgl_lock();
        lv_tick_inc(elapsed_ms);
        lv_task_handler();
        lvgl_unlock();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}