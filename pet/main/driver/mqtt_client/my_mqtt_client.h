#ifndef MY_MQTT_CLIENT_H
#define MY_MQTT_CLIENT_H
#include <stdint.h>              // for int32_t
#include "esp_event.h"          // for esp_event_base_t
#include "mqtt_client.h"        // for esp_mqtt_event_handle_t if used

#define TUYA_DEVICE_ID      "269765034aa743092eh9fq"
#define TUYA_CLIENT_ID      "tuyalink_"TUYA_DEVICE_ID
#define TUYA_MQTT_URI       "mqtts://m1.tuyacn.com:8883?clientid="TUYA_CLIENT_ID

#define TUYA_DEVICE_SECRET   "fzUUrL4SHI8gsPo5"
#define TUYA_TOPIC_PUBLISH   "tylink/"TUYA_DEVICE_ID"/thing/property/report"
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void my_hmac_sha256(const char *key, const char *data, char *output_hex);
char *build_json_payload(void);
void tuya_mqtt_start(void);
#endif