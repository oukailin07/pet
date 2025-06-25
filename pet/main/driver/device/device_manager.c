#include "device_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "app_spiffs.h"

static const char *TAG = "DEVICE_MANAGER";

#define DEVICE_INFO_PATH "/spiffs/device_info.json"

// NVS命名空间和键名
#define DEVICE_NAMESPACE "device"
#define DEVICE_ID_KEY "device_id"
#define DEVICE_PASSWORD_KEY "password"

// 设备信息
static char device_id[64] = {0};
static char device_password[64] = {0};
static bool device_initialized = false;
static bool wifi_connected = false;

// 心跳包定时器
static TimerHandle_t heartbeat_timer = NULL;

// 心跳包间隔（5分钟）
#define HEARTBEAT_INTERVAL_MS (5 * 60 * 1000)

// 前向声明
static esp_err_t save_device_info_to_spiffs(void);
static esp_err_t load_device_info_from_spiffs(void);

// 替换NVS相关函数为SPIFFS实现
#define save_device_info_to_nvs save_device_info_to_spiffs
#define load_device_info_from_nvs load_device_info_from_spiffs

/**
 * @brief 心跳包响应处理函数
 */
static void heartbeat_response_handler(bool need_device_id, const char *new_device_id, const char *new_password)
{
    if (need_device_id && new_device_id && new_password) {
        ESP_LOGI(TAG, "Parsed device_id: %s, password: %s", new_device_id, new_password);
        // 保存新的设备ID和密码
        strncpy(device_id, new_device_id, sizeof(device_id) - 1);
        device_id[sizeof(device_id) - 1] = '\0';
        
        strncpy(device_password, new_password, sizeof(device_password) - 1);
        device_password[sizeof(device_password) - 1] = '\0';

        ESP_LOGI(TAG, "Received new device ID: %s", device_id);
        ESP_LOGI(TAG, "Received new password");

        // 保存到SPIFFS
        if (save_device_info_to_spiffs() == ESP_OK) {
            device_initialized = true;
            ESP_LOGI(TAG, "Device initialized successfully");
        }
    } else {
        ESP_LOGI(TAG, "Heartbeat successful, device already initialized");
    }
}

/**
 * @brief 从SPIFFS读取设备信息
 */
static esp_err_t load_device_info_from_spiffs(void)
{
    FILE* f = fopen(DEVICE_INFO_PATH, "r");
    if (!f) {
        ESP_LOGI(TAG, "Device info file not found in SPIFFS, will request from server");
        device_id[0] = '\0';
        device_password[0] = '\0';
        return ESP_FAIL;
    }
    char buf[256] = {0};
    size_t read = fread(buf, 1, sizeof(buf)-1, f);
    fclose(f);
    if (read == 0) {
        ESP_LOGE(TAG, "Failed to read device info from SPIFFS");
        return ESP_FAIL;
    }
    cJSON* root = cJSON_Parse(buf);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse device info JSON");
        return ESP_FAIL;
    }
    const cJSON* id = cJSON_GetObjectItem(root, "device_id");
    const cJSON* pwd = cJSON_GetObjectItem(root, "device_password");
    if (id && id->valuestring) {
        strncpy(device_id, id->valuestring, sizeof(device_id)-1);
        device_id[sizeof(device_id)-1] = '\0';
    } else {
        device_id[0] = '\0';
    }
    if (pwd && pwd->valuestring) {
        strncpy(device_password, pwd->valuestring, sizeof(device_password)-1);
        device_password[sizeof(device_password)-1] = '\0';
    } else {
        device_password[0] = '\0';
    }
    cJSON_Delete(root);
    ESP_LOGI(TAG, "Loaded device info from SPIFFS: %s", device_id);
    return ESP_OK;
}

/**
 * @brief 保存设备信息到SPIFFS
 */
static esp_err_t save_device_info_to_spiffs(void)
{
    FILE* f = fopen(DEVICE_INFO_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", DEVICE_INFO_PATH);
        return ESP_FAIL;
    }
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id);
    cJSON_AddStringToObject(root, "device_password", device_password);
    char* json_str = cJSON_PrintUnformatted(root);
    size_t written = fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);
    cJSON_Delete(root);
    free(json_str);
    if (written == 0) {
        ESP_LOGE(TAG, "Failed to write device info to SPIFFS");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Device info saved to SPIFFS successfully");
    return ESP_OK;
}

/**
 * @brief 发送心跳包
 */
static esp_err_t send_heartbeat_with_response(void)
{
    ESP_LOGI(TAG, "send_heartbeat_with_response called");
    if (!wifi_connected) {
        ESP_LOGW(TAG, "WiFi not connected, skipping heartbeat");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending heartbeat with device ID: %s", device_id[0] ? device_id : "NULL");
    return send_heartbeat(device_id[0] ? device_id : NULL);
}

/**
 * @brief 心跳包定时器回调函数
 */
static void heartbeat_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Sending heartbeat...");
    send_heartbeat_with_response();
}

/**
 * @brief 启动心跳包定时器
 */
static esp_err_t start_heartbeat_timer(void)
{
    if (heartbeat_timer != NULL) {
        ESP_LOGW(TAG, "Heartbeat timer already exists");
        return ESP_OK;
    }

    heartbeat_timer = xTimerCreate(
        "heartbeat_timer",
        pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS),
        pdTRUE,  // 自动重载
        NULL,
        heartbeat_timer_callback
    );

    if (heartbeat_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create heartbeat timer");
        return ESP_FAIL;
    }

    if (xTimerStart(heartbeat_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start heartbeat timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Heartbeat timer started, interval: %d ms", HEARTBEAT_INTERVAL_MS);
    return ESP_OK;
}

/**
 * @brief 停止心跳包定时器
 */
static esp_err_t stop_heartbeat_timer(void)
{
    if (heartbeat_timer != NULL) {
        xTimerStop(heartbeat_timer, 0);
        xTimerDelete(heartbeat_timer, 0);
        heartbeat_timer = NULL;
        ESP_LOGI(TAG, "Heartbeat timer stopped");
    }
    return ESP_OK;
}

// 公共接口函数实现

esp_err_t device_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing device manager...");

    // 加载设备信息（不管成不成功都继续）
    load_device_info_from_spiffs();

    // 检查设备是否已初始化
    if (device_id[0] != '\0') {
        device_initialized = true;
        ESP_LOGI(TAG, "Device already initialized with ID: %s", device_id);
    } else {
        ESP_LOGI(TAG, "Device not initialized, will request ID from server");
    }

    // 设置HTTP客户端心跳包响应回调
    http_client_set_heartbeat_callback(heartbeat_response_handler);

    ESP_LOGI(TAG, "Device manager initialized successfully");
    return ESP_OK;
}

esp_err_t device_manager_start_heartbeat(void)
{
    if (!wifi_connected) {
        ESP_LOGW(TAG, "WiFi not connected, cannot start heartbeat");
        return ESP_FAIL;
    }

    // 立即发送一次心跳包
    ESP_LOGI(TAG, "Sending initial heartbeat...");
    send_heartbeat_with_response();

    // 启动定时器
    return start_heartbeat_timer();
}

esp_err_t device_manager_stop_heartbeat(void)
{
    return stop_heartbeat_timer();
}

void device_manager_set_wifi_status(bool connected)
{
    wifi_connected = connected;
    ESP_LOGI(TAG, "WiFi status changed: %s", connected ? "connected" : "disconnected");
    
    if (connected) {
        // WiFi连接后启动心跳包
        device_manager_start_heartbeat();
    } else {
        // WiFi断开后停止心跳包
        device_manager_stop_heartbeat();
    }
}

const char* device_manager_get_device_id(void)
{
    return device_id;
}

const char* device_manager_get_device_password(void)
{
    return device_password;
}

bool device_manager_is_initialized(void)
{
    return device_initialized;
}

esp_err_t device_manager_force_heartbeat(void)
{
    ESP_LOGI(TAG, "Force sending heartbeat...");
    return send_heartbeat_with_response();
} 