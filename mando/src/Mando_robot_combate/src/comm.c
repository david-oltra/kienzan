/* comm.c */

#include "comm.h"
#include "app_config.h"
#include "remote_types.h"
#include "joysticks.h"

#include <string.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char *TAG = "COMM";

/* ---------- Configuración ---------- */
#define ESPNOW_CHANNEL      1
#define ESPNOW_TX_PERIOD_MS 20     /* 50 Hz */

/* MAC del receptor (el barco). */
static uint8_t s_peer_mac[6] = {0xa0, 0xf2, 0x62, 0xa9, 0xe4, 0x98};

/* ---------- Estado interno ---------- */

static bool              s_ready     = false;
static SemaphoreHandle_t s_rx_mutex  = NULL;
static uint32_t          s_tx_count  = 0;
static uint32_t          s_rx_count  = 0;

static received_data_t   s_received  = { .gps_month = 1, .gps_day = 1 };

/* ---------- WiFi ---------- */

static esp_err_t wifi_setup(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    ESP_LOGI(TAG, "WiFi STA ready on channel %d", ESPNOW_CHANNEL);
    ESP_LOGI(TAG, "MAC (STA): " MACSTR, MAC2STR(mac));

    return ESP_OK;
}

/* ---------- Callbacks ESP-NOW ---------- */

static void on_recv(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len)
{
    (void)info;

    if (len != (int)sizeof(received_data_t)) {
        return;
    }

    if (xSemaphoreTake(s_rx_mutex, portMAX_DELAY) == pdTRUE) {
        memcpy(&s_received, data, sizeof(s_received));
        s_rx_count++;
        xSemaphoreGive(s_rx_mutex);
    }
}

static void on_send(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    (void)mac_addr;
    (void)status;
    s_tx_count++;
}

/* ---------- Peer management ---------- */

static esp_err_t peer_add(const uint8_t mac[6])
{
    esp_now_peer_info_t info = {0};
    info.channel = ESPNOW_CHANNEL;
    info.ifidx   = WIFI_IF_STA;
    info.encrypt = false;
    memcpy(info.peer_addr, mac, 6);

    if (esp_now_is_peer_exist(mac)) {
        esp_now_del_peer(mac);
    }

    esp_err_t err = esp_now_add_peer(&info);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add_peer failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "peer " MACSTR " added", MAC2STR(mac));
    }
    return err;
}

/* ---------- API pública ---------- */

esp_err_t comm_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    s_rx_mutex = xSemaphoreCreateMutex();
    if (s_rx_mutex == NULL) {
        ESP_LOGE(TAG, "mutex alloc failed");
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(wifi_setup());

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_send));

    ESP_ERROR_CHECK(peer_add(s_peer_mac));

    s_ready = true;
    ESP_LOGI(TAG, "ESP-NOW ready");
    return ESP_OK;
}

void comm_task(void *arg)
{
    (void)arg;

    while (1) {
        if (!s_ready || !joysticks_is_armed()) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        esp_now_send(s_peer_mac,
                     (const uint8_t *)&remote_data,
                     sizeof(remote_data));

        vTaskDelay(pdMS_TO_TICKS(ESPNOW_TX_PERIOD_MS));
    }
}

esp_err_t comm_get_received(received_data_t *out)
{
    if (out == NULL)  return ESP_ERR_INVALID_ARG;
    if (!s_ready)     return ESP_ERR_INVALID_STATE;

    if (xSemaphoreTake(s_rx_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }
    memcpy(out, &s_received, sizeof(*out));
    xSemaphoreGive(s_rx_mutex);

    return ESP_OK;
}

esp_err_t comm_set_peer(const uint8_t mac[6])
{
    if (mac == NULL) return ESP_ERR_INVALID_ARG;

    memcpy(s_peer_mac, mac, 6);

    if (s_ready) {
        return peer_add(s_peer_mac);
    }
    return ESP_OK;
}

void comm_get_stats(uint32_t *tx, uint32_t *rx)
{
    if (tx) *tx = s_tx_count;
    if (rx) *rx = s_rx_count;
}