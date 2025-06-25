#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 心跳包响应回调函数类型定义
typedef void (*heartbeat_response_callback_t)(bool need_device_id, const char *device_id, const char *password);

/**
 * @brief 发送心跳包
 * @param device_id 设备ID，可以为NULL
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t send_heartbeat(const char *device_id);

/**
 * @brief 设置心跳包响应回调函数
 * @param callback 回调函数指针
 */
void http_client_set_heartbeat_callback(heartbeat_response_callback_t callback);

/**
 * @brief 获取粮桶重量
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t get_grain_level(void);

/**
 * @brief 通过HTTP添加喂食计划到服务器
 * @param device_id 设备ID
 * @param day_of_week 星期 (0=星期日, 1=星期一, ..., 6=星期六)
 * @param hour 小时 (0-23)
 * @param minute 分钟 (0-59)
 * @param feeding_amount 喂食量 (克)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t http_add_feeding_plan(const char *device_id, int day_of_week, int hour, int minute, float feeding_amount);

/**
 * @brief 发送粮桶重量
 * @param device_id 设备ID
 * @param grain_weight 粮桶重量
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t send_grain_weight(const char *device_id, float grain_weight);

/**
 * @brief 发送手动喂食记录
 * @param device_id 设备ID
 * @param hour 小时 (0-23)
 * @param minute 分钟 (0-59)
 * @param feeding_amount 喂食量 (克)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t send_manual_feeding(const char *device_id, int hour, int minute, float feeding_amount);

/**
 * @brief 发送喂食记录
 * @param device_id 设备ID
 * @param day_of_week 星期 (0=星期日, 1=星期一, ..., 6=星期六)
 * @param hour 小时 (0-23)
 * @param minute 分钟 (0-59)
 * @param feeding_amount 喂食量 (克)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t send_feeding_record(const char *device_id, int day_of_week, int hour, int minute, float feeding_amount);

#ifdef __cplusplus
}
#endif

#endif // HTTP_CLIENT_H