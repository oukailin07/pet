#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化设备管理器
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t device_manager_init(void);

/**
 * @brief 启动心跳包定时器
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t device_manager_start_heartbeat(void);

/**
 * @brief 停止心跳包定时器
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t device_manager_stop_heartbeat(void);

/**
 * @brief 设置WiFi连接状态
 * @param connected WiFi连接状态
 */
void device_manager_set_wifi_status(bool connected);

/**
 * @brief 获取设备ID
 * @return const char* 设备ID字符串，未初始化时返回空字符串
 */
const char* device_manager_get_device_id(void);

/**
 * @brief 获取设备密码
 * @return const char* 设备密码字符串，未初始化时返回空字符串
 */
const char* device_manager_get_device_password(void);

/**
 * @brief 检查设备是否已初始化
 * @return bool true表示已初始化，false表示未初始化
 */
bool device_manager_is_initialized(void);

/**
 * @brief 强制发送心跳包
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t device_manager_force_heartbeat(void);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_MANAGER_H 