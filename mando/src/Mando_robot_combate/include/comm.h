/* comm.h
 *
 * WiFi STA + ESP-NOW.
 * - comm_task: envía remote_data al peer a frecuencia fija.
 * - recv_cb:   rellena received_data (protegida por mutex).
 *
 * El peer por defecto es broadcast. Se puede cambiar en runtime
 * con comm_set_peer().
 */

#pragma once

#include <stdint.h>
#include <esp_err.h>
#include "remote_types.h"

/**
 * Inicializa WiFi en modo STA, arranca ESP-NOW, registra callbacks
 * y añade el peer por defecto.
 */
esp_err_t comm_init(void);

/**
 * Tarea de envío periódico. Bucle infinito.
 */
void comm_task(void *arg);

/**
 * Copia received_data a *out de forma thread-safe.
 * Devuelve ESP_OK, o ESP_ERR_INVALID_STATE si comm aún no está listo.
 */
esp_err_t comm_get_received(received_data_t *out);

/**
 * Cambia el peer de destino (por ejemplo, tras leerlo de NVS).
 * Se puede llamar antes o después de comm_init().
 */
esp_err_t comm_set_peer(const uint8_t mac[6]);

/**
 * Información de diagnóstico: cuántos paquetes enviados/recibidos
 * desde el último reset. Útil para la pantalla de estado.
 */
void comm_get_stats(uint32_t *tx, uint32_t *rx);