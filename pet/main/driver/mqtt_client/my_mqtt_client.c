#include "my_mqtt_client.h"
#include "mqtt_client.h"
#include "mbedtls/md.h"
#include <time.h>
#include "esp_log.h"
#include "cJSON.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mbedtls/md.h"
#include "cJSON.h"

extern const uint8_t _binary_tuya_root_cert_pem_start[];  // 注意名字！
extern const uint8_t _binary_tuya_root_cert_pem_end[];

static const char *TAG = "TUYA_MQTT";
esp_mqtt_client_handle_t mqtt_client = NULL;
static const char *ca_crt = NULL;

void my_hmac_sha256(const char *key, const char *data, char *output_hex)
{
    unsigned char hmac[32];
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, info, 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)data, strlen(data));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    for (int i = 0; i < 32; ++i)
        sprintf(output_hex + i * 2, "%02x", hmac[i]);
    output_hex[64] = '\0';
}


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            break;
        default:
            break;
    }
}


char *build_json_payload(void)
{
    time_t t = time(NULL);
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%lld", (long long)t);  // 兼容性更好

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "msgID", timestamp);
    cJSON_AddStringToObject(root, "time", timestamp);

    cJSON *data = cJSON_CreateObject();
    cJSON *temp = cJSON_CreateObject();
    cJSON_AddNumberToObject(temp, "value", 25);  // 改为数值
    cJSON_AddStringToObject(temp, "time", timestamp);
    cJSON_AddItemToObject(data, "temperature", temp);

    cJSON_AddItemToObject(root, "data", data);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

char *build_json_payload1(void)
{
    time_t t = time(NULL);
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%lld", (long long)t);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "msgID", timestamp);
    cJSON_AddStringToObject(root, "time", timestamp);

    cJSON *data = cJSON_CreateObject();
    cJSON *temp = cJSON_CreateObject();
    cJSON_AddNumberToObject(temp, "value", 1000);  // 改为数值
    cJSON_AddStringToObject(temp, "time", timestamp);
    cJSON_AddItemToObject(data, "BUCKET", temp);

    cJSON_AddItemToObject(root, "data", data);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

static void mqtt_publish_task(void *arg)
{
    while (1) {
        char *payload = build_json_payload();
        esp_mqtt_client_publish(mqtt_client, TUYA_TOPIC_PUBLISH, payload, 0, 0, 0);
        ESP_LOGI(TAG, "Published: %s", payload);
        vTaskDelay(pdMS_TO_TICKS(1000));
        char *payload1 = build_json_payload1();
        esp_mqtt_client_publish(mqtt_client, TUYA_TOPIC_PUBLISH, payload1, 0, 0, 0);
        ESP_LOGI(TAG, "Published: %s", payload1);
        free(payload);
        free(payload1);
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void tuya_mqtt_start(void)
{
    char username[256], password[65], sign_src[256];

    time_t now = time(NULL);
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%lld", now);

    snprintf(username, sizeof(username), "%s|signMethod=hmacSha256,timestamp=%s,secureMode=1,accessType=1", TUYA_DEVICE_ID, timestamp);
    snprintf(sign_src, sizeof(sign_src), "deviceId=%s,timestamp=%s,secureMode=1,accessType=1", TUYA_DEVICE_ID, timestamp);
    my_hmac_sha256(TUYA_DEVICE_SECRET, sign_src, password);
    ESP_LOGI(TAG, "ClientID: %s", TUYA_CLIENT_ID);
    ESP_LOGI(TAG, "MQTT URI: %s", TUYA_MQTT_URI);
    ESP_LOGI(TAG, "Username: %s", username);
    ESP_LOGI(TAG, "Password: %s", password);
    ESP_LOGI(TAG, "Using root cert: %.*s", 64, _binary_tuya_root_cert_pem_start);
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = TUYA_MQTT_URI,
            .verification.certificate = (const char *)_binary_tuya_root_cert_pem_start,
        },
        .credentials = {
            .client_id = TUYA_CLIENT_ID,
            .username = username,
            .authentication.password = password,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    xTaskCreatePinnedToCore(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 7, NULL,1);
}