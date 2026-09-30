/* ui.h
 *
 * Interfaz de la capa de presentación.
 * - ui_init():       construye todas las pantallas y carga el estado de NVS.
 * - ui_task():       consume la cola de buttons, navega, actualiza datos.
 * - ui_lvgl_task():  tick + handler de LVGL.
 *
 * Todas las llamadas a LVGL desde cualquier tarea están serializadas
 * por un mutex interno.
 */

#pragma once

#include <esp_err.h>

esp_err_t ui_init(void);
void      ui_task(void *arg);
void      ui_lvgl_task(void *arg);