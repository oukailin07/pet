#ifndef FEEDING_MANAGER_H
#define FEEDING_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 最大喂食计划数量
#define MAX_FEEDING_PLANS 20
#define MAX_MANUAL_FEEDINGS 10

/**
 * @brief 喂食计划结构体
 */
typedef struct {
    int id;                 // 计划ID
    int day_of_week;        // 星期 (0=星期日, 1=星期一, ..., 6=星期六)
    int hour;               // 小时 (0-23)
    int minute;             // 分钟 (0-59)
    float feeding_amount;   // 喂食量 (克)
    bool enabled;           // 是否启用
} feeding_plan_t;

/**
 * @brief 手动喂食结构体
 */
typedef struct {
    int id;                 // 喂食ID
    int hour;               // 小时 (0-23)
    int minute;             // 分钟 (0-59)
    float feeding_amount;   // 喂食量 (克)
    bool executed;          // 是否已执行
} manual_feeding_t;

/**
 * @brief 初始化喂食管理系统
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t feeding_manager_init(void);

/**
 * @brief 添加定时喂食计划
 * @param day_of_week 星期 (0=星期日, 1=星期一, ..., 6=星期六)
 * @param hour 小时 (0-23)
 * @param minute 分钟 (0-59)
 * @param feeding_amount 喂食量 (克)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t add_feeding_plan(int day_of_week, int hour, int minute, float feeding_amount);

/**
 * @brief 添加手动喂食
 * @param hour 小时 (0-23)
 * @param minute 分钟 (0-59)
 * @param feeding_amount 喂食量 (克)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t add_manual_feeding(int hour, int minute, float feeding_amount);

/**
 * @brief 删除喂食计划
 * @param plan_id 计划ID
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t delete_feeding_plan(int plan_id);

/**
 * @brief 启用/禁用喂食计划
 * @param plan_id 计划ID
 * @param enabled 是否启用
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t enable_feeding_plan(int plan_id, bool enabled);

/**
 * @brief 获取喂食计划数量
 * @return int 计划数量
 */
int get_feeding_plans_count(void);

/**
 * @brief 获取手动喂食数量
 * @return int 手动喂食数量
 */
int get_manual_feedings_count(void);

/**
 * @brief 获取指定索引的喂食计划
 * @param index 索引
 * @return feeding_plan_t* 喂食计划指针，失败返回NULL
 */
feeding_plan_t* get_feeding_plan(int index);

/**
 * @brief 获取指定索引的手动喂食
 * @param index 索引
 * @return manual_feeding_t* 手动喂食指针，失败返回NULL
 */
manual_feeding_t* get_manual_feeding(int index);

/**
 * @brief 执行立即喂食
 * @param feeding_amount 喂食量 (克)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t execute_immediate_feeding(float feeding_amount);

#ifdef __cplusplus
}
#endif

#endif // FEEDING_MANAGER_H 