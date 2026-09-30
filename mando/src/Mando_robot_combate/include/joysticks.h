/* joysticks.h
 *
 * Lectura de los 4 ejes analógicos del mando.
 * - ADC oneshot promediado.
 * - Calibración guiada, persistida en NVS.
 * - Tarea que escribe remote_data a frecuencia fija.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/* Callback opcional para reportar progreso durante la calibración.
 * ui lo usa para actualizar el label. Puede ser NULL. */
typedef void (*joy_progress_cb_t)(const char *msg);

/**
 * Configura el ADC y carga la calibración desde NVS.
 * Si no hay calibración guardada, aplica valores por defecto (0, 4095).
 */
esp_err_t joysticks_init(void);

/**
 * Tarea de lectura continua. Bucle infinito.
 * Escribe en remote_data. Si se está calibrando, se queda esperando.
 */
void joysticks_task(void *arg);

/**
 * Ejecuta la calibración guiada de los 4 ejes.
 *
 * BLOQUEANTE. Debe llamarse desde ui_task.
 * Mientras corre, joysticks_task no lee el ADC.
 * Los mensajes de progreso se pasan por `cb` (puede ser NULL).
 *
 * Al terminar, guarda los valores en NVS, recarga el estado interno
 * y devuelve el control.
 */
void joysticks_calibrate(joy_progress_cb_t cb);

/**
 * ¿Hay una calibración en curso?
 * Útil para que ui ignore los botones mientras dura.
 */
bool joysticks_is_calibrating(void);

/**
 * ¿Está el mando armado? Es decir, ¿se ha comprobado ya que el
 * joystick izquierdo Y está a 0 desde el arranque?
 *
 * Mientras devuelve false:
 *   - comm_task NO debe enviar paquetes ESP-NOW.
 *   - ui debe mostrar el aviso de seguridad.
 *
 * Pasa a true la primera vez que el eje Y izquierdo llega a ~0 y ya
 * no vuelve a false hasta el siguiente reinicio.
 */
bool      joysticks_is_armed(void);