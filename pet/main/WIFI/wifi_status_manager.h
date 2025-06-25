#ifndef WIFI_STATUS_MANAGER_H
#define WIFI_STATUS_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// WiFi状态回调函数类型定义
typedef void (*wifi_status_callback_t)(bool connected);

/**
 * @brief 获取当前WiFi连接状态
 * @return bool true表示已连接，false表示未连接
 */
bool wifi_status_manager_is_connected(void);

/**
 * @brief 获取当前WiFi信号强度
 * @return int8_t 信号强度(dBm)，连接失败时返回-100
 */
int8_t wifi_status_manager_get_rssi(void);

/**
 * @brief 新的统一启动函数
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @param callback WiFi状态变化回调函数
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t wifi_status_manager_start(const char* ssid, const char* password, wifi_status_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // WIFI_STATUS_MANAGER_H 