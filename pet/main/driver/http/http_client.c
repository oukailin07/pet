#include "http_client.h"
#include <string.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_http_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_log.h"
#include "cJSON.h"
// HTTP 客户端配置
esp_http_client_handle_t client;
#define SERVER_URL "http://192.168.0.101:80"  // Flask server URL

// 心跳包响应回调函数指针
static heartbeat_response_callback_t heartbeat_response_callback = NULL;

/**
 * @brief 设置心跳包响应回调函数
 */
void http_client_set_heartbeat_callback(heartbeat_response_callback_t callback)
{
    heartbeat_response_callback = callback;
}

/**
 * @brief 解析心跳包响应
 */
static esp_err_t parse_heartbeat_response(const char *response_data, size_t response_len)
{
    if (response_data == NULL || response_len == 0) {
        ESP_LOGE("HTTP_CLIENT", "Invalid response data");
        return ESP_FAIL;
    }

    // 解析JSON响应
    cJSON *root = cJSON_Parse(response_data);
    if (root == NULL) {
        ESP_LOGE("HTTP_CLIENT", "Failed to parse JSON response");
        return ESP_FAIL;
    }

    // 检查是否需要分配设备ID
    cJSON *need_device_id = cJSON_GetObjectItem(root, "need_device_id");
    if (need_device_id && cJSON_IsTrue(need_device_id)) {
        // 服务器需要分配设备ID
        cJSON *new_device_id = cJSON_GetObjectItem(root, "device_id");
        cJSON *new_password = cJSON_GetObjectItem(root, "password");

        if (new_device_id && new_password && 
            cJSON_IsString(new_device_id) && cJSON_IsString(new_password)) {
            
            ESP_LOGI("HTTP_CLIENT", "Received new device ID: %s", new_device_id->valuestring);
            
            // 调用回调函数
            if (heartbeat_response_callback) {
                heartbeat_response_callback(true, new_device_id->valuestring, new_password->valuestring);
            }
        }
    } else {
        // 检查设备ID是否有效
        cJSON *status = cJSON_GetObjectItem(root, "status");
        if (status && cJSON_IsString(status)) {
            if (strcmp(status->valuestring, "success") == 0) {
                ESP_LOGI("HTTP_CLIENT", "Heartbeat successful");
                if (heartbeat_response_callback) {
                    heartbeat_response_callback(false, NULL, NULL);
                }
            } else {
                ESP_LOGW("HTTP_CLIENT", "Heartbeat status: %s", status->valuestring);
            }
        }
    }

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t send_heartbeat(const char *device_id) {
    ESP_LOGI("HTTP_CLIENT", "send_heartbeat called with device_id: %s", device_id ? device_id : "NULL");
    esp_http_client_config_t config = {
        .url = SERVER_URL "/device/heartbeat",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,  // 10秒超时
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id ? device_id : "");
    cJSON_AddStringToObject(root, "device_type", "pet_feeder");
    cJSON_AddStringToObject(root, "firmware_version", "1.0.0");
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    ESP_LOGI("HTTP_CLIENT", "send_heartbeat result: %d", err);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI("Heartbeat", "Status Code: %d", status_code);

        if (status_code == 200) {
            // 获取响应数据
            int content_length = esp_http_client_get_content_length(client);
            if (content_length > 0) {
                char *response_data = malloc(content_length + 1);
                if (response_data) {
                    int read_len = esp_http_client_read(client, response_data, content_length);
                    response_data[read_len] = '\0';
                    
                    ESP_LOGI("Heartbeat", "Response: %s", response_data);
                    parse_heartbeat_response(response_data, read_len);
                    
                    free(response_data);
                }
            }
        }
    } else {
        ESP_LOGE("Heartbeat", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(json_data);

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t get_grain_level() {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/get_grain_level",
        .method = HTTP_METHOD_GET,
    };

    client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("Get Grain Level", "Status Code: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("Get Grain Level", "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t http_add_feeding_plan(const char *device_id, int day_of_week, int hour, int minute, float feeding_amount) {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/add_feeding_plan",
        .method = HTTP_METHOD_POST,
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id);
    cJSON_AddNumberToObject(root, "day_of_week", day_of_week);
    cJSON_AddNumberToObject(root, "hour", hour);
    cJSON_AddNumberToObject(root, "minute", minute);
    cJSON_AddNumberToObject(root, "feeding_amount", feeding_amount);
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("Add Feeding Plan", "Status Code: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("Add Feeding Plan", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(json_data);

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t send_grain_weight(const char *device_id, float grain_weight) {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/upload_grain_level",  // 服务器的上传接口
        .method = HTTP_METHOD_POST,
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id ? device_id : "");
    cJSON_AddNumberToObject(root, "grain_level", grain_weight);  // 粮桶重量
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI("Grain Weight", "POST JSON: %s", json_data);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("Grain Weight", "Status Code: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("Grain Weight", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(json_data);

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t send_manual_feeding(const char *device_id, int hour, int minute, float feeding_amount) {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/manual_feeding",  // 服务器的上传手动喂食记录接口
        .method = HTTP_METHOD_POST,
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id);
    cJSON_AddNumberToObject(root, "hour", hour);
    cJSON_AddNumberToObject(root, "minute", minute);
    cJSON_AddNumberToObject(root, "feeding_amount", feeding_amount);
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("Manual Feeding", "Status Code: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("Manual Feeding", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(json_data);

    esp_http_client_cleanup(client);
    return err;
}

esp_err_t send_feeding_record(const char *device_id, int day_of_week, int hour, int minute, float feeding_amount) {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/add_feeding_record",  // 服务器的上传喂食记录接口
        .method = HTTP_METHOD_POST,
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id);
    cJSON_AddNumberToObject(root, "day_of_week", day_of_week);
    cJSON_AddNumberToObject(root, "hour", hour);
    cJSON_AddNumberToObject(root, "minute", minute);
    cJSON_AddNumberToObject(root, "feeding_amount", feeding_amount);
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("Feeding Record", "Status Code: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("Feeding Record", "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    cJSON_Delete(root);
    free(json_data);

    esp_http_client_cleanup(client);
    return err;
}