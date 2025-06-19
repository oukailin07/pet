#ifndef WS2812_H
#define WS2812_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_log.h"
#include "stdio.h"
#include "math.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define LED_STRIP_USE_DMA 0
#define LED_STRIP_GPIO_PIN 7
#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_RMT_RES_HZ 10000000
#define LED_STRIP_MEMORY_BLOCK_WORDS 0

#define MATRIX_WIDTH  16
#define MATRIX_HEIGHT 8

led_strip_handle_t configure_led(void);



typedef struct {
    bool is_light;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t led_max_num;
    uint8_t led_ctrl_num;
    uint8_t command;
} app_led_config_t;

void app_led_init(gpio_num_t pin, int max_num);
#endif