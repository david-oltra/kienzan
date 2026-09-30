/* buttons.h
 *
 * Gestión de los 6 pulsadores físicos (3 funciones lógicas x 2 lados).
 *
 * - ISR mínima: solo debounce por timestamp + encolar evento lógico.
 * - Cero llamadas a LVGL desde contexto de interrupción.
 * - La cola se consume desde ui_task().
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

/* Acciones lógicas del mando */
typedef enum {
    BTN_UP = 0,   /* subir pantalla / subir en menú */
    BTN_DOWN,     /* bajar pantalla / bajar en menú */
    BTN_OK,       /* entrar / confirmar */
    BTN_COUNT
} button_id_t;

/* Evento que viaja por la cola */
typedef struct {
    button_id_t id;
} button_event_t;

/**
 * Configura GPIOs, instala el ISR service y crea la cola de eventos.
 * Debe llamarse una sola vez, antes de ui_task.
 */
esp_err_t buttons_init(void);

/**
 * Devuelve la cola de eventos. ui_task hace xQueueReceive sobre ella.
 * Nunca devuelve NULL si buttons_init() tuvo éxito.
 */
QueueHandle_t buttons_get_queue(void);