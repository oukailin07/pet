#include "wifi_status_manager.h"
#include "device_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#define TAG "WIFI_STATUS"
static int s_retry_num = 0; // retry number
// WiFi状态回调函数指针
static wifi_status_callback_t wifi_status_callback = NULL;
EventGroupHandle_t app_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
/**
 * @brief WiFi事件处理函数
 */
static void app_wifi_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            //esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(app_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(app_wifi_event_group, WIFI_CONNECTED_BIT);
        if (wifi_status_callback) {
            wifi_status_callback(true);
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_status_callback) {
            wifi_status_callback(false);
        }
    }
}

/**
 * @brief 新的统一启动函数
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @param callback WiFi状态变化回调函数
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t wifi_status_manager_start(const char* ssid, const char* password, wifi_status_callback_t callback)
{
    // 保存回调
    wifi_status_callback = callback;

    app_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &app_wifi_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &app_wifi_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA_PSK,
        },
    };
    strncpy((char *)&wifi_config.sta.ssid, ssid, strlen(ssid));
    strncpy((char *)&wifi_config.sta.password, password, strlen(password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(app_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }
}

/**
 * @brief 获取当前WiFi连接状态
 * @return bool true表示已连接，false表示未连接
 */
bool wifi_status_manager_is_connected(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    return (ret == ESP_OK);
}

/**
 * @brief 获取当前WiFi信号强度
 * @return int8_t 信号强度(dBm)，连接失败时返回-100
 */
int8_t wifi_status_manager_get_rssi(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK) {
        return ap_info.rssi;
    }
    return -100;  // 默认值，表示连接失败
} 