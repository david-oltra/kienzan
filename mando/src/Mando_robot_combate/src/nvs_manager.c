/* nvs_manager.c */

#include "nvs_manager.h"
#include "app_config.h"

#include <string.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char *TAG = "NVS";

static nvs_handle_t      s_handle   = 0;
static SemaphoreHandle_t s_mutex    = NULL;
static bool              s_ready    = false;

/* ---------- helpers internos ---------- */

static inline esp_err_t lock(void)
{
    return xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE
           ? ESP_OK : ESP_FAIL;
}

static inline void unlock(void)
{
    xSemaphoreGive(s_mutex);
}

/* ---------- API pública ---------- */

esp_err_t nvs_manager_init(void)
{
    if (s_ready) {
        ESP_LOGW(TAG, "already initialised");
        return ESP_OK;
    }

    /* 1. Init del subsistema NVS (con recuperación si la partición está corrupta) */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase (%s)", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return err;
    }

    /* 2. Mutex */
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        ESP_LOGE(TAG, "mutex alloc failed");
        return ESP_ERR_NO_MEM;
    }

    /* 3. Abrir namespace de la app */
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &s_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open('%s') failed: %s", NVS_NAMESPACE, esp_err_to_name(err));
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
        return err;
    }

    s_ready = true;
    ESP_LOGI(TAG, "NVS ready (namespace '%s')", NVS_NAMESPACE);
    return ESP_OK;
}

esp_err_t nvs_manager_deinit(void)
{
    if (!s_ready) return ESP_OK;

    if (lock() != ESP_OK) return ESP_FAIL;

    nvs_close(s_handle);
    s_handle = 0;
    s_ready  = false;

    unlock();

    vSemaphoreDelete(s_mutex);
    s_mutex = NULL;
    return ESP_OK;
}

/* ---------- uint8_t ---------- */

esp_err_t nvs_manager_get_u8(const char *key, uint8_t *out)
{
    if (!s_ready || key == NULL || out == NULL) return ESP_ERR_INVALID_ARG;
    if (lock() != ESP_OK) return ESP_FAIL;

    esp_err_t err = nvs_get_u8(s_handle, key, out);

    unlock();

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        /* No lo tratamos como error de log: es un caso normal en primer arranque */
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "get_u8('%s') failed: %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_manager_set_u8(const char *key, uint8_t value)
{
    if (!s_ready || key == NULL) return ESP_ERR_INVALID_ARG;
    if (lock() != ESP_OK) return ESP_FAIL;

    esp_err_t err = nvs_set_u8(s_handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(s_handle);
    }

    unlock();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_u8('%s', %u) failed: %s", key, value, esp_err_to_name(err));
    }
    return err;
}

/* ---------- int16_t ---------- */

esp_err_t nvs_manager_get_i16(const char *key, int16_t *out)
{
    if (!s_ready || key == NULL || out == NULL) return ESP_ERR_INVALID_ARG;
    if (lock() != ESP_OK) return ESP_FAIL;

    esp_err_t err = nvs_get_i16(s_handle, key, out);

    unlock();

    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_ERR_NVS_NOT_FOUND;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "get_i16('%s') failed: %s", key, esp_err_to_name(err));
    }
    return err;
}

esp_err_t nvs_manager_set_i16(const char *key, int16_t value)
{
    if (!s_ready || key == NULL) return ESP_ERR_INVALID_ARG;
    if (lock() != ESP_OK) return ESP_FAIL;

    esp_err_t err = nvs_set_i16(s_handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(s_handle);
    }

    unlock();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_i16('%s', %d) failed: %s", key, value, esp_err_to_name(err));
    }
    return err;
}

/* ---------- utilidades ---------- */

esp_err_t nvs_manager_erase_all(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    if (lock() != ESP_OK) return ESP_FAIL;

    esp_err_t err = nvs_erase_all(s_handle);
    if (err == ESP_OK) {
        err = nvs_commit(s_handle);
    }

    unlock();

    if (err == ESP_OK) ESP_LOGI(TAG, "namespace '%s' erased", NVS_NAMESPACE);
    else               ESP_LOGE(TAG, "erase_all failed: %s", esp_err_to_name(err));
    return err;
}