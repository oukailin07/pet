#ifndef WS2812_H
#define WS2812_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#define LED_STRIP_USE_DMA 0
#define LED_STRIP_GPIO_PIN 7
#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_RMT_RES_HZ 10000000
#define LED_STRIP_MEMORY_BLOCK_WORDS 0

#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 8

led_strip_handle_t configure_led(void);
#endif