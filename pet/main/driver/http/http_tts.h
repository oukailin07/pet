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

void http_get_ip_tts(char *ip);
#endif // HTTP_TTS_H