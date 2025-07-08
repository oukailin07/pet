#ifndef HTTP_TTS_H
#define HTTP_TTS_H

#include "esp_err.h"
#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "string.h"
#include "app_url_encode.h"
#include "esp_http_client.h"

// 不同的MP3文件路径
#define MP3_SAVE_PATH_IP "/spiffs/baidu_tts_ip.mp3"
#define MP3_SAVE_PATH_DEVICE_ID "/spiffs/baidu_tts_device_id.mp3"
#define MP3_SAVE_PATH_PASSWORD "/spiffs/baidu_tts_password.mp3"
#define MP3_SAVE_PATH_CUSTOM "/spiffs/baidu_tts_custom.mp3"
#define MP3_SAVE_PATH_DEFAULT "/spiffs/baidu_tts.mp3"

// 原有函数（保持向后兼容）
void http_get_ip_tts(char *ip);

// 新的函数声明
void http_get_device_id_tts(char *device_id);
void http_get_password_tts(char *password);
void http_get_custom_tts(const char *text);

// 支持自定义文件名的函数
void http_get_ip_tts_with_filename(char *ip, const char *filename);
void http_get_device_id_tts_with_filename(char *device_id, const char *filename);
void http_get_password_tts_with_filename(char *password, const char *filename);
void http_get_custom_tts_with_filename(const char *text, const char *filename);

#endif // HTTP_TTS_H