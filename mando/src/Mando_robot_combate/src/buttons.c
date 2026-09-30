/* buttons.c */

#include "buttons.h"
#include "app_config.h"

#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static const char *TAG = "BTN";

/* ---------- Configuración de pines ---------- */

typedef struct {
    gpio_num_t  pin;
    button_id_t id;
} button_map_t;

static const button_map_t s_buttons[] = {
    { PIN_BTN_UP_LEFT,    BTN_UP   },
    { PIN_BTN_UP_RIGHT,   BTN_UP   },
    { PIN_BTN_OK_LEFT,    BTN_OK   },
    { PIN_BTN_OK_RIGHT,   BTN_OK   },
    { PIN_BTN_DOWN_LEFT,  BTN_DOWN },
    { PIN_BTN_DOWN_RIGHT, BTN_DOWN },
};

#define N_BUTTONS (sizeof(s_buttons) / sizeof(s_buttons[0]))

/* ---------- Estado interno ---------- */

static QueueHandle_t s_queue = NULL;

/* Un timestamp por botón físico (no por función lógica),
 * así el rebote de un botón no bloquea al otro del mismo lado. */
static volatile int64_t s_last_press_us[N_BUTTONS] = {0};

/* ---------- ISR ---------- */

static IRAM_ATTR void button_isr(void *arg)
{
    /* arg es un puntero al elemento de s_buttons[] correspondiente */
    const button_map_t *btn = (const button_map_t *)arg;
    int idx = (int)(btn - s_buttons);

    int64_t now = esp_timer_get_time();
    if (now - s_last_press_us[idx] < BTN_DEBOUNCE_US) {
        return;                     /* rebote: descartar */
    }
    s_last_press_us[idx] = now;

    button_event_t ev = { .id = btn->id };

    BaseType_t hp_task_woken = pdFALSE;
    xQueueSendFromISR(s_queue, &ev, &hp_task_woken);
    if (hp_task_woken) portYIELD_FROM_ISR();
}

/* ---------- API pública ---------- */

esp_err_t buttons_init(void)
{
    if (s_queue != NULL) {
        ESP_LOGW(TAG, "already initialised");
        return ESP_OK;
    }

    /* 1. Cola de eventos */
    s_queue = xQueueCreate(16, sizeof(button_event_t));
    if (s_queue == NULL) {
        ESP_LOGE(TAG, "queue alloc failed");
        return ESP_ERR_NO_MEM;
    }

    /* 2. Configuración GPIO de todos los pines a la vez */
    uint64_t mask = 0;
    for (size_t i = 0; i < N_BUTTONS; i++) {
        mask |= (1ULL << s_buttons[i].pin);
    }

    gpio_config_t cfg = {
        .pin_bit_mask = mask,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,   /* igual que el original */
        .intr_type    = GPIO_INTR_POSEDGE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(err));
        vQueueDelete(s_queue);
        s_queue = NULL;
        return err;
    }

    /* 3. ISR service. Flags de IRAM para que la ISR no toque flash. */
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE = ya estaba instalado; lo aceptamos */
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(err));
        vQueueDelete(s_queue);
        s_queue = NULL;
        return err;
    }

    /* 4. Enganchar cada pin a la ISR, pasando su entrada como arg */
    for (size_t i = 0; i < N_BUTTONS; i++) {
        err = gpio_isr_handler_add(s_buttons[i].pin, button_isr,
                                   (void *)&s_buttons[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "isr_handler_add(%d) failed: %s",
                     s_buttons[i].pin, esp_err_to_name(err));
            return err;
        }
    }

    ESP_LOGI(TAG, "%u buttons ready", (unsigned)N_BUTTONS);
    return ESP_OK;
}

QueueHandle_t buttons_get_queue(void)
{
    return s_queue;
}