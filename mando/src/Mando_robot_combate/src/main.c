/* main.c
 * Punto de entrada del mando.
 * Solo orquesta: inicializa módulos y arranca tareas.
 */

#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app_config.h"
#include "nvs_manager.h"
#include "joysticks.h"
#include "buttons.h"
#include "comm.h"
#include "ui.h"

static const char *TAG = "MAIN";

/* ---------- Orquestadores de init ---------- */
static void init_peripherals(void)
{
    ESP_ERROR_CHECK(nvs_manager_init());
    ESP_ERROR_CHECK(joysticks_init());
    ESP_ERROR_CHECK(buttons_init());
}

static void init_comms(void)
{
    ESP_ERROR_CHECK(comm_init());
}

static void init_display(void)
{
    ESP_ERROR_CHECK(ui_init());
}

/* ---------- Arranque de tareas ---------- */
static void start_tasks(void)
{
    BaseType_t ok;

    ok = xTaskCreatePinnedToCore(ui_task,        "ui",
                                 TASK_STACK_SIZE, NULL,
                                 TASK_UI_PRIO,    NULL,
                                 TASK_UI_CORE);
    configASSERT(ok == pdPASS);

    ok = xTaskCreatePinnedToCore(ui_lvgl_task,   "lvgl",
                                 TASK_STACK_SIZE, NULL,
                                 TASK_LVGL_PRIO,  NULL,
                                 TASK_LVGL_CORE);
    configASSERT(ok == pdPASS);

    ok = xTaskCreatePinnedToCore(joysticks_task, "joysticks",
                                 TASK_STACK_SIZE, NULL,
                                 TASK_JOY_PRIO,   NULL,
                                 TASK_JOY_CORE);
    configASSERT(ok == pdPASS);

    ok = xTaskCreatePinnedToCore(comm_task,      "comm",
                                 TASK_STACK_SIZE, NULL,
                                 TASK_COMM_PRIO,  NULL,
                                 TASK_COMM_CORE);
    configASSERT(ok == pdPASS);
}

/* ---------- Entry point ---------- */
void app_main(void)
{
    ESP_LOGI(TAG, "Remote starting...");

    init_peripherals();
    init_comms();
    init_display();

    start_tasks();

    ESP_LOGI(TAG, "Remote up and running");
    /* app_main retorna; el scheduler sigue con las tareas creadas */
}