#include "feeding_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include "time_utils.h"
#include "motor.h"
#include "hx711.h"
#include "http_client.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "FEEDING_MANAGER";

// 喂食计划文件路径
#define FEEDING_PLANS_FILE "/spiffs/feeding_plans.json"
#define MANUAL_FEEDING_FILE "/spiffs/manual_feeding.json"
#define FEEDING_RECORDS_FILE "/spiffs/feeding_records.json"

// 全局变量
static feeding_plan_t feeding_plans[MAX_FEEDING_PLANS];
static manual_feeding_t manual_feedings[MAX_MANUAL_FEEDINGS];
static int plan_count = 0;
static int manual_count = 0;
static bool system_initialized = false;

// 设备ID（可以从配置或NVS中读取）
static char device_id[32] = "ESP32_PET_001";

/**
 * @brief 保存喂食计划到SPIFFS
 */
static esp_err_t save_feeding_plans_to_spiffs(void)
{
    FILE *file = fopen(FEEDING_PLANS_FILE, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open feeding plans file for writing");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *plans_array = cJSON_CreateArray();

    for (int i = 0; i < plan_count; i++) {
        cJSON *plan = cJSON_CreateObject();
        cJSON_AddNumberToObject(plan, "id", feeding_plans[i].id);
        cJSON_AddNumberToObject(plan, "day_of_week", feeding_plans[i].day_of_week);
        cJSON_AddNumberToObject(plan, "hour", feeding_plans[i].hour);
        cJSON_AddNumberToObject(plan, "minute", feeding_plans[i].minute);
        cJSON_AddNumberToObject(plan, "feeding_amount", feeding_plans[i].feeding_amount);
        cJSON_AddBoolToObject(plan, "enabled", feeding_plans[i].enabled);
        cJSON_AddItemToArray(plans_array, plan);
    }

    cJSON_AddItemToObject(root, "plans", plans_array);
    char *json_string = cJSON_Print(root);
    
    if (json_string != NULL) {
        fwrite(json_string, 1, strlen(json_string), file);
        free(json_string);
    }

    cJSON_Delete(root);
    fclose(file);
    
    ESP_LOGI(TAG, "Feeding plans saved to SPIFFS");
    return ESP_OK;
}

/**
 * @brief 从SPIFFS加载喂食计划
 */
static esp_err_t load_feeding_plans_from_spiffs(void)
{
    FILE *file = fopen(FEEDING_PLANS_FILE, "r");
    if (file == NULL) {
        ESP_LOGI(TAG, "No feeding plans file found, starting with empty plans");
        return ESP_OK;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return ESP_OK;
    }

    // 读取文件内容
    char *json_buffer = malloc(file_size + 1);
    if (json_buffer == NULL) {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(json_buffer, 1, file_size, file);
    json_buffer[bytes_read] = '\0';
    fclose(file);

    // 解析JSON
    cJSON *root = cJSON_Parse(json_buffer);
    free(json_buffer);

    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse feeding plans JSON");
        return ESP_FAIL;
    }

    cJSON *plans_array = cJSON_GetObjectItem(root, "plans");
    if (plans_array == NULL || !cJSON_IsArray(plans_array)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    plan_count = 0;
    int array_size = cJSON_GetArraySize(plans_array);
    
    for (int i = 0; i < array_size && plan_count < MAX_FEEDING_PLANS; i++) {
        cJSON *plan_item = cJSON_GetArrayItem(plans_array, i);
        if (plan_item != NULL) {
            feeding_plans[plan_count].id = cJSON_GetObjectItem(plan_item, "id")->valueint;
            feeding_plans[plan_count].day_of_week = cJSON_GetObjectItem(plan_item, "day_of_week")->valueint;
            feeding_plans[plan_count].hour = cJSON_GetObjectItem(plan_item, "hour")->valueint;
            feeding_plans[plan_count].minute = cJSON_GetObjectItem(plan_item, "minute")->valueint;
            feeding_plans[plan_count].feeding_amount = cJSON_GetObjectItem(plan_item, "feeding_amount")->valuedouble;
            feeding_plans[plan_count].enabled = cJSON_GetObjectItem(plan_item, "enabled")->valueint;
            plan_count++;
        }
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Loaded %d feeding plans from SPIFFS", plan_count);
    return ESP_OK;
}

/**
 * @brief 保存手动喂食到SPIFFS
 */
static esp_err_t save_manual_feedings_to_spiffs(void)
{
    FILE *file = fopen(MANUAL_FEEDING_FILE, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open manual feeding file for writing");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *feedings_array = cJSON_CreateArray();

    for (int i = 0; i < manual_count; i++) {
        cJSON *feeding = cJSON_CreateObject();
        cJSON_AddNumberToObject(feeding, "id", manual_feedings[i].id);
        cJSON_AddNumberToObject(feeding, "hour", manual_feedings[i].hour);
        cJSON_AddNumberToObject(feeding, "minute", manual_feedings[i].minute);
        cJSON_AddNumberToObject(feeding, "feeding_amount", manual_feedings[i].feeding_amount);
        cJSON_AddBoolToObject(feeding, "executed", manual_feedings[i].executed);
        cJSON_AddItemToArray(feedings_array, feeding);
    }

    cJSON_AddItemToObject(root, "feedings", feedings_array);
    char *json_string = cJSON_Print(root);
    
    if (json_string != NULL) {
        fwrite(json_string, 1, strlen(json_string), file);
        free(json_string);
    }

    cJSON_Delete(root);
    fclose(file);
    
    ESP_LOGI(TAG, "Manual feedings saved to SPIFFS");
    return ESP_OK;
}

/**
 * @brief 从SPIFFS加载手动喂食
 */
static esp_err_t load_manual_feedings_from_spiffs(void)
{
    FILE *file = fopen(MANUAL_FEEDING_FILE, "r");
    if (file == NULL) {
        ESP_LOGI(TAG, "No manual feeding file found, starting with empty feedings");
        return ESP_OK;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        return ESP_OK;
    }

    // 读取文件内容
    char *json_buffer = malloc(file_size + 1);
    if (json_buffer == NULL) {
        fclose(file);
        return ESP_ERR_NO_MEM;
    }

    size_t bytes_read = fread(json_buffer, 1, file_size, file);
    json_buffer[bytes_read] = '\0';
    fclose(file);

    // 解析JSON
    cJSON *root = cJSON_Parse(json_buffer);
    free(json_buffer);

    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse manual feeding JSON");
        return ESP_FAIL;
    }

    cJSON *feedings_array = cJSON_GetObjectItem(root, "feedings");
    if (feedings_array == NULL || !cJSON_IsArray(feedings_array)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    manual_count = 0;
    int array_size = cJSON_GetArraySize(feedings_array);
    
    for (int i = 0; i < array_size && manual_count < MAX_MANUAL_FEEDINGS; i++) {
        cJSON *feeding_item = cJSON_GetArrayItem(feedings_array, i);
        if (feeding_item != NULL) {
            manual_feedings[manual_count].id = cJSON_GetObjectItem(feeding_item, "id")->valueint;
            manual_feedings[manual_count].hour = cJSON_GetObjectItem(feeding_item, "hour")->valueint;
            manual_feedings[manual_count].minute = cJSON_GetObjectItem(feeding_item, "minute")->valueint;
            manual_feedings[manual_count].feeding_amount = cJSON_GetObjectItem(feeding_item, "feeding_amount")->valuedouble;
            manual_feedings[manual_count].executed = cJSON_GetObjectItem(feeding_item, "executed")->valueint;
            manual_count++;
        }
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Loaded %d manual feedings from SPIFFS", manual_count);
    return ESP_OK;
}

/**
 * @brief 执行喂食操作
 */
static esp_err_t execute_feeding(float target_amount)
{
    ESP_LOGI(TAG, "开始执行喂食，目标重量: %.2f g", target_amount);
    
    // 获取初始重量
    float initial_weight = HX711_get_units(5);
    ESP_LOGI(TAG, "初始重量: %.2f g", initial_weight);
    
    // 启动电机
    motor_control(MOTOR_FORWARD);
    ESP_LOGI(TAG, "电机启动，开始出粮");
    
    // 监控重量变化
    float current_weight = initial_weight;
    float last_weight = initial_weight;
    int no_change_count = 0;
    const int max_no_change = 10; // 连续10次重量无变化则停止
    
    while (current_weight < (initial_weight + target_amount)) {
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms检查一次
        
        current_weight = HX711_get_units(3); // 读取3次平均值
        
        // 检查重量是否还在增加
        if (fabs(current_weight - last_weight) < 0.5) { // 0.5g的误差范围
            no_change_count++;
        } else {
            no_change_count = 0;
        }
        
        // 如果连续多次重量无变化，可能卡住了
        if (no_change_count >= max_no_change) {
            ESP_LOGW(TAG, "重量连续无变化，可能卡住，停止电机");
            break;
        }
        
        last_weight = current_weight;
        
        // 每500ms打印一次进度
        static int progress_counter = 0;
        if (++progress_counter % 5 == 0) {
            float progress = (current_weight - initial_weight) / target_amount * 100;
            ESP_LOGI(TAG, "喂食进度: %.1f%%, 当前重量: %.2f g", progress, current_weight);
        }
    }
    
    // 停止电机
    motor_control(MOTOR_STOP);
    ESP_LOGI(TAG, "电机停止");
    
    // 计算实际喂食量
    float actual_amount = current_weight - initial_weight;
    ESP_LOGI(TAG, "喂食完成，实际喂食量: %.2f g", actual_amount);
    
    return ESP_OK;
}

/**
 * @brief 上报喂食记录
 */
static esp_err_t report_feeding_record(int day_of_week, int hour, int minute, float feeding_amount)
{
    ESP_LOGI(TAG, "上报喂食记录: 星期%d %02d:%02d 喂食量%.2fg", 
             day_of_week, hour, minute, feeding_amount);
    
    return send_feeding_record(device_id, day_of_week, hour, minute, feeding_amount);
}

/**
 * @brief 检查并执行定时喂食计划
 */
static void check_scheduled_feeding(void)
{
    time_info_t current_time;
    if (get_current_time_info(&current_time) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get current time");
        return;
    }
    
    for (int i = 0; i < plan_count; i++) {
        if (!feeding_plans[i].enabled) {
            continue;
        }
        
        // 检查是否是当前时间
        if (feeding_plans[i].day_of_week == current_time.weekday &&
            feeding_plans[i].hour == current_time.hour &&
            feeding_plans[i].minute == current_time.minute) {
            
            ESP_LOGI(TAG, "执行定时喂食计划 ID: %d", feeding_plans[i].id);
            
            // 执行喂食
            if (execute_feeding(feeding_plans[i].feeding_amount) == ESP_OK) {
                // 上报喂食记录
                report_feeding_record(current_time.weekday, current_time.hour, 
                                    current_time.minute, feeding_plans[i].feeding_amount);
            }
        }
    }
}

/**
 * @brief 检查并执行手动喂食
 */
static void check_manual_feeding(void)
{
    time_info_t current_time;
    if (get_current_time_info(&current_time) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get current time");
        return;
    }
    
    for (int i = 0; i < manual_count; i++) {
        if (manual_feedings[i].executed) {
            continue;
        }
        
        // 检查是否是当前时间
        if (manual_feedings[i].hour == current_time.hour &&
            manual_feedings[i].minute == current_time.minute) {
            
            ESP_LOGI(TAG, "执行手动喂食 ID: %d", manual_feedings[i].id);
            
            // 执行喂食
            if (execute_feeding(manual_feedings[i].feeding_amount) == ESP_OK) {
                // 上报手动喂食记录
                send_manual_feeding(device_id, current_time.hour, current_time.minute, 
                                  manual_feedings[i].feeding_amount);
                
                // 标记为已执行
                manual_feedings[i].executed = true;
                
                // 保存到SPIFFS
                save_manual_feedings_to_spiffs();
                
                // 删除已执行的手动喂食
                for (int j = i; j < manual_count - 1; j++) {
                    manual_feedings[j] = manual_feedings[j + 1];
                }
                manual_count--;
                
                ESP_LOGI(TAG, "手动喂食执行完成并删除");
            }
        }
    }
}

/**
 * @brief 喂食管理任务
 */
static void feeding_manager_task(void *pvParameters)
{
    ESP_LOGI(TAG, "喂食管理任务启动");
    
    while (1) {
        if (system_initialized) {
            // 检查定时喂食计划
            check_scheduled_feeding();
            
            // 检查手动喂食
            check_manual_feeding();
        }
        
        // 每分钟检查一次
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

// 公共接口函数实现

esp_err_t feeding_manager_init(void)
{
    ESP_LOGI(TAG, "初始化喂食管理系统");
    
    // 加载喂食计划
    if (load_feeding_plans_from_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load feeding plans");
        return ESP_FAIL;
    }
    
    // 加载手动喂食
    if (load_manual_feedings_from_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load manual feedings");
        return ESP_FAIL;
    }
    
    system_initialized = true;
    
    // 启动喂食管理任务
    xTaskCreate(feeding_manager_task, "feeding_manager", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "喂食管理系统初始化完成");
    return ESP_OK;
}

esp_err_t add_feeding_plan(int day_of_week, int hour, int minute, float feeding_amount)
{
    if (plan_count >= MAX_FEEDING_PLANS) {
        ESP_LOGE(TAG, "喂食计划数量已达上限");
        return ESP_FAIL;
    }
    
    // 生成唯一ID
    static int next_id = 1;
    feeding_plans[plan_count].id = next_id++;
    feeding_plans[plan_count].day_of_week = day_of_week;
    feeding_plans[plan_count].hour = hour;
    feeding_plans[plan_count].minute = minute;
    feeding_plans[plan_count].feeding_amount = feeding_amount;
    feeding_plans[plan_count].enabled = true;
    
    plan_count++;
    
    // 保存到SPIFFS
    if (save_feeding_plans_to_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save feeding plan to SPIFFS");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "添加喂食计划成功: 星期%d %02d:%02d %.2fg", 
             day_of_week, hour, minute, feeding_amount);
    
    return ESP_OK;
}

esp_err_t add_manual_feeding(int hour, int minute, float feeding_amount)
{
    if (manual_count >= MAX_MANUAL_FEEDINGS) {
        ESP_LOGE(TAG, "手动喂食数量已达上限");
        return ESP_FAIL;
    }
    
    // 生成唯一ID
    static int next_manual_id = 1;
    manual_feedings[manual_count].id = next_manual_id++;
    manual_feedings[manual_count].hour = hour;
    manual_feedings[manual_count].minute = minute;
    manual_feedings[manual_count].feeding_amount = feeding_amount;
    manual_feedings[manual_count].executed = false;
    
    manual_count++;
    
    // 保存到SPIFFS
    if (save_manual_feedings_to_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save manual feeding to SPIFFS");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "添加手动喂食成功: %02d:%02d %.2fg", hour, minute, feeding_amount);
    
    return ESP_OK;
}

esp_err_t delete_feeding_plan(int plan_id)
{
    for (int i = 0; i < plan_count; i++) {
        if (feeding_plans[i].id == plan_id) {
            // 删除计划
            for (int j = i; j < plan_count - 1; j++) {
                feeding_plans[j] = feeding_plans[j + 1];
            }
            plan_count--;
            
            // 保存到SPIFFS
            if (save_feeding_plans_to_spiffs() != ESP_OK) {
                ESP_LOGE(TAG, "Failed to save feeding plans after deletion");
                return ESP_FAIL;
            }
            
            ESP_LOGI(TAG, "删除喂食计划成功: ID %d", plan_id);
            return ESP_OK;
        }
    }
    
    ESP_LOGE(TAG, "未找到要删除的喂食计划: ID %d", plan_id);
    return ESP_FAIL;
}

esp_err_t enable_feeding_plan(int plan_id, bool enabled)
{
    for (int i = 0; i < plan_count; i++) {
        if (feeding_plans[i].id == plan_id) {
            feeding_plans[i].enabled = enabled;
            
            // 保存到SPIFFS
            if (save_feeding_plans_to_spiffs() != ESP_OK) {
                ESP_LOGE(TAG, "Failed to save feeding plans after enabling/disabling");
                return ESP_FAIL;
            }
            
            ESP_LOGI(TAG, "%s喂食计划: ID %d", enabled ? "启用" : "禁用", plan_id);
            return ESP_OK;
        }
    }
    
    ESP_LOGE(TAG, "未找到要修改的喂食计划: ID %d", plan_id);
    return ESP_FAIL;
}

int get_feeding_plans_count(void)
{
    return plan_count;
}

int get_manual_feedings_count(void)
{
    return manual_count;
}

feeding_plan_t* get_feeding_plan(int index)
{
    if (index >= 0 && index < plan_count) {
        return &feeding_plans[index];
    }
    return NULL;
}

manual_feeding_t* get_manual_feeding(int index)
{
    if (index >= 0 && index < manual_count) {
        return &manual_feedings[index];
    }
    return NULL;
}

esp_err_t execute_immediate_feeding(float feeding_amount)
{
    ESP_LOGI(TAG, "执行立即喂食: %.2f g", feeding_amount);
    
    if (execute_feeding(feeding_amount) == ESP_OK) {
        time_info_t current_time;
        if (get_current_time_info(&current_time) == ESP_OK) {
            // 上报立即喂食记录
            send_manual_feeding(device_id, current_time.hour, current_time.minute, feeding_amount);
        }
        return ESP_OK;
    }
    
    return ESP_FAIL;
} 