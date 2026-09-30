/* nvs_manager.h
 *
 * Envoltorio mínimo sobre NVS.
 * - Un solo handle abierto durante toda la vida de la app.
 * - Acceso thread-safe (mutex interno).
 * - Tipos concretos: u8 y i16 (los que usa el proyecto).
 * - set_* hace commit automático.
 */

#pragma once

#include <stdint.h>
#include <esp_err.h>
#include <nvs.h>

/**
 * Inicializa NVS y abre el namespace de la app.
 * Debe llamarse una sola vez, antes que cualquier otra función.
 * Es idempotente: llamarla dos veces no hace daño.
 */
esp_err_t nvs_manager_init(void);

/**
 * Cierra el handle NVS. Normalmente no hace falta llamarla,
 * pero queda disponible por limpieza.
 */
esp_err_t nvs_manager_deinit(void);

/* ---------- uint8_t ---------- */

/**
 * Lee un uint8_t.
 * @return ESP_OK                    valor leído
 *         ESP_ERR_NVS_NOT_FOUND     la clave no existe (out intacto)
 *         otro                      error de NVS
 */
esp_err_t nvs_manager_get_u8(const char *key, uint8_t *out);

/**
 * Escribe un uint8_t y hace commit.
 */
esp_err_t nvs_manager_set_u8(const char *key, uint8_t value);

/* ---------- int16_t ---------- */

esp_err_t nvs_manager_get_i16(const char *key, int16_t *out);
esp_err_t nvs_manager_set_i16(const char *key, int16_t value);

/* ---------- utilidades ---------- */

/**
 * Borra todas las claves del namespace de la app.
 * Útil para "reset factory" desde el menú.
 */
esp_err_t nvs_manager_erase_all(void);