/* app_config.h
 *
 * Constantes globales del proyecto: pines, canales ADC, rangos lógicos,
 * prioridades de tareas y enumeración de pantallas.
 */

#pragma once

#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>

/* ---------- Pines de botones ---------- */
#define PIN_BTN_UP_LEFT     GPIO_NUM_32
#define PIN_BTN_OK_LEFT     GPIO_NUM_33
#define PIN_BTN_DOWN_LEFT   GPIO_NUM_25
#define PIN_BTN_UP_RIGHT    GPIO_NUM_26
#define PIN_BTN_OK_RIGHT    GPIO_NUM_27
#define PIN_BTN_DOWN_RIGHT  GPIO_NUM_12

/* ---------- Canales ADC ---------- */
#define JOY_RV_CH   ADC_CHANNEL_0
#define JOY_RH_CH   ADC_CHANNEL_3
#define JOY_LV_CH   ADC_CHANNEL_6
#define JOY_LH_CH   ADC_CHANNEL_7
#define JOY_ADC_UNIT        ADC_UNIT_1
#define JOY_ADC_BITWIDTH    ADC_BITWIDTH_12
#define JOY_ADC_ATTEN       ADC_ATTEN_DB_11

/* ---------- Rangos lógicos de los ejes ---------- */
/* Ejes bipolares: -100 (extremo bajo) .. 0 (centro) .. +100 (extremo alto) */
#define JOY_BIP_MIN         (-100)
#define JOY_BIP_MAX         ( 100)

/* Eje unipolar (joystick izquierdo vertical, sin centro) */
#define JOY_UNI_MIN         (0)
#define JOY_UNI_MAX         (100)

/* ---------- Detección de extremos durante calibración ---------- */
#define JOY_CAL_LOW_DETECT  3500   /* umbral para detectar extremo "bajo" */
#define JOY_CAL_HIGH_DETECT 500    /* umbral para detectar extremo "alto" */

/* ---------- Debounce botones ---------- */
#define BTN_DEBOUNCE_US     300000  /* 300 ms en microsegundos */

/* ---------- Tareas ---------- */
#define TASK_LVGL_CORE      0
#define TASK_UI_CORE        0
#define TASK_JOY_CORE       1
#define TASK_COMM_CORE      1
#define TASK_LVGL_PRIO      8
#define TASK_UI_PRIO        9
#define TASK_JOY_PRIO       10
#define TASK_COMM_PRIO      9
#define TASK_STACK_SIZE     4096

/* ---------- NVS ---------- */
#define NVS_NAMESPACE       "storage"

/* ---------- Enumeración de pantallas ---------- */
typedef enum {
    SCR_HOME = 0,
    SCR_HOME_CHART,
    SCR_PANEL,
    SCR_MOTOR,
    SCR_BATERIA,
    SCR_GPS,
    SCR_RELOJ,
    SCR_COUNT,
    SCR_MENU = SCR_COUNT,
    SCR_MENU_CONFIG,
} screen_id_t;