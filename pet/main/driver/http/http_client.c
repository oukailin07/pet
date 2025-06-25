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
#define SERVER_URL "http://192.168.10.101:80"  // Flask server URL
// 定义请求类型
typedef enum {
    HTTP_REQ_NONE = 0,
    HTTP_REQ_HEARTBEAT,
    HTTP_REQ_GRAIN_WEIGHT,
    // 其他请求类型可继续添加
} http_request_type_t;

// 全局状态结构体
static struct {
    http_request_type_t current_type;
    char response_buf[1024];
    int response_len;
} http_req_ctx = {0};

// 独立回调类型和回调函数指针
typedef void (*heartbeat_response_callback_t)(bool need_device_id, const char *device_id, const char *password);
static heartbeat_response_callback_t heartbeat_response_callback = NULL;

typedef void (*grain_weight_response_callback_t)(esp_err_t result, const char *msg);
static grain_weight_response_callback_t grain_weight_response_callback = NULL;

/**
 * @brief 设置心跳包响应回调函数
 */
void http_client_set_heartbeat_callback(heartbeat_response_callback_t callback)
{
    heartbeat_response_callback = callback;
}

void http_client_set_grain_weight_callback(grain_weight_response_callback_t callback) {
    grain_weight_response_callback = callback;
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

// 解析上传粮食重量响应
static esp_err_t parse_grain_weight_response(const char *response_data, size_t response_len)
{
    if (response_data == NULL || response_len == 0) {
        ESP_LOGE("Grain Weight", "Invalid response data");
        return ESP_FAIL;
    }
    cJSON *root = cJSON_Parse(response_data);
    if (root == NULL) {
        ESP_LOGE("Grain Weight", "Failed to parse JSON response");
        return ESP_FAIL;
    }
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (status && cJSON_IsString(status)) {
        if (strcmp(status->valuestring, "success") == 0) {
            if (grain_weight_response_callback) {
                grain_weight_response_callback(ESP_OK, "上传粮食重量成功");
            }
        } else {
            if (grain_weight_response_callback) {
                grain_weight_response_callback(ESP_FAIL, status->valuestring);
            }
        }
    } else {
        if (grain_weight_response_callback) {
            grain_weight_response_callback(ESP_FAIL, "无status字段");
        }
    }
    cJSON_Delete(root);
    return ESP_OK;
}

// 事件回调
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // 累积数据
                int copy_len = evt->data_len;
                if (http_req_ctx.response_len + copy_len >= sizeof(http_req_ctx.response_buf)) {
                    copy_len = sizeof(http_req_ctx.response_buf) - http_req_ctx.response_len - 1;
                }
                if (copy_len > 0) {
                    memcpy(http_req_ctx.response_buf + http_req_ctx.response_len, evt->data, copy_len);
                    http_req_ctx.response_len += copy_len;
                    http_req_ctx.response_buf[http_req_ctx.response_len] = '\0';
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            // 数据接收完毕，解析
            if (http_req_ctx.current_type == HTTP_REQ_HEARTBEAT) {
                parse_heartbeat_response(http_req_ctx.response_buf, http_req_ctx.response_len);
            } else if (http_req_ctx.current_type == HTTP_REQ_GRAIN_WEIGHT) {
                parse_grain_weight_response(http_req_ctx.response_buf, http_req_ctx.response_len);
            }
            // 清空缓冲区
            http_req_ctx.response_len = 0;
            http_req_ctx.response_buf[0] = '\0';
            http_req_ctx.current_type = HTTP_REQ_NONE;
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t send_heartbeat(const char *device_id, heartbeat_response_callback_t callback) {
    http_req_ctx.current_type = HTTP_REQ_HEARTBEAT;
    http_req_ctx.response_len = 0;
    http_req_ctx.response_buf[0] = '\0';
    http_client_set_heartbeat_callback(callback);
    esp_http_client_config_t config = {
        .url = SERVER_URL "/device/heartbeat",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .event_handler = _http_event_handler,
    };
    client = esp_http_client_init(&config);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id ? device_id : "");
    cJSON_AddStringToObject(root, "device_type", "pet_feeder");
    cJSON_AddStringToObject(root, "firmware_version", "1.0.0");
    char *json_data = cJSON_Print(root);
    ESP_LOGI("HTTP_CLIENT", "POST JSON: %s", json_data);
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_err_t err = esp_http_client_perform(client);
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

esp_err_t send_grain_weight(const char *device_id, float grain_weight, grain_weight_response_callback_t callback) {
    http_req_ctx.current_type = HTTP_REQ_GRAIN_WEIGHT;
    http_req_ctx.response_len = 0;
    http_req_ctx.response_buf[0] = '\0';
    http_client_set_grain_weight_callback(callback);
    esp_http_client_config_t config = {
        .url = SERVER_URL "/upload_grain_level",
        .method = HTTP_METHOD_POST,
        .event_handler = _http_event_handler,
    };
    client = esp_http_client_init(&config);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id ? device_id : "");
    cJSON_AddNumberToObject(root, "grain_level", grain_weight);
    char *json_data = cJSON_Print(root);
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");
    ESP_LOGI("Grain Weight", "POST JSON: %s", json_data);
    esp_err_t err = esp_http_client_perform(client);
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