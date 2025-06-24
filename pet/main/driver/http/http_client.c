#include "http_client.h"
#include <string.h>
#include <stdio.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>
#include <esp_http_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_log.h"
#include "cJSON.h"
// HTTP 客户端配置
esp_http_client_handle_t client;
#define SERVER_URL "http://127.0.0.1:80"  // Flask server URL
esp_err_t send_heartbeat(const char *device_id) {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/device/heartbeat",
        .method = HTTP_METHOD_POST,
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "device_id", device_id);
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("Heartbeat", "Status Code: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE("Heartbeat", "HTTP GET request failed: %s", esp_err_to_name(err));
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

esp_err_t add_feeding_plan(const char *device_id, int day_of_week, int hour, int minute, float feeding_amount) {
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

esp_err_t send_grain_weight(float grain_weight) {
    esp_http_client_config_t config = {
        .url = SERVER_URL "/upload_grain_level",  // 服务器的上传接口
        .method = HTTP_METHOD_POST,
    };

    client = esp_http_client_init(&config);

    // 构建请求体 JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "grain_level", grain_weight);  // 粮桶重量
    char *json_data = cJSON_Print(root);

    esp_http_client_set_post_field(client, json_data, strlen(json_data));

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