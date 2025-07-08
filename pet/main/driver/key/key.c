#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "key.h"
#include "hal_i2s.h"
#include "hx711.h"
#define KEY_GPIO                    0
#define DEBOUNCE_TIME_MS           30
#define DOUBLE_CLICK_INTERVAL_MS   300
#define LONG_PRESS_TIME_MS         3000

static QueueHandle_t gpio_evt_queue = NULL;
static esp_timer_handle_t click_timer;  // 新增定时器
typedef struct {
    int64_t last_press_time;
    int64_t last_release_time;
    int click_count;
    bool is_pressed;
    bool long_press_reported;
} button_state_t;

static button_state_t btn_state = {0};
// 定时器回调函数
static void click_timer_callback(void* arg)
{
    if (btn_state.click_count == 1) {
        button_event_callback(BUTTON_EVENT_SINGLE_CLICK);
    } else if (btn_state.click_count == 2) {
        button_event_callback(BUTTON_EVENT_DOUBLE_CLICK);
    }
    btn_state.click_count = 0;
}

// 创建定时器
static void create_click_timer(void)
{
    esp_timer_create_args_t timer_args = {
        .callback = &click_timer_callback,
        .arg = NULL,
        .name = "click_timer"
    };
    esp_timer_create(&timer_args, &click_timer);
}
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
void button_event_callback(button_event_t event)
{
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            printf("==> 单击切换模式\n");
            hx711_trigger_calibration();
            ESP_ERROR_CHECK(audio_app_player_music_queue("/spiffs/out_one_food.mp3"));
            break;
        case BUTTON_EVENT_DOUBLE_CLICK:
            printf("==> 双击进入配网模式或显示IP\n");
            break;
        case BUTTON_EVENT_LONG_PRESS:
            printf("==> 长按清除WiFi配置\n");
            break;
    }
}



static void key_task(void* arg)
{
    uint32_t io_num;
    int last_level = 1;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));
            int level = gpio_get_level(io_num);
            int64_t now = esp_timer_get_time() / 1000;

            if (level == 0 && last_level == 1) {
                // 按下
                btn_state.is_pressed = true;
                btn_state.last_press_time = now;
                btn_state.long_press_reported = false;

            } else if (level == 1 && last_level == 0) {
                // 松开
                btn_state.is_pressed = false;
                int press_duration = now - btn_state.last_press_time;

                if (press_duration >= LONG_PRESS_TIME_MS) {
                    if (!btn_state.long_press_reported) {
                        button_event_callback(BUTTON_EVENT_LONG_PRESS);
                        btn_state.long_press_reported = true;
                        btn_state.click_count = 0;  // 清除其他点击状态
                    }
                } else {
                    btn_state.click_count++;
                    // 重启定时器，每次点击都会重新计时
                    esp_timer_stop(click_timer);
                    esp_timer_start_once(click_timer, DOUBLE_CLICK_INTERVAL_MS * 1000); // us
                }
            }

            last_level = level;
        }
    }
}


void key_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << KEY_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(KEY_GPIO, gpio_isr_handler, (void*)KEY_GPIO);
create_click_timer(); 
    xTaskCreatePinnedToCore(key_task, "key_task", 2048, NULL, 5, NULL, 1);
}

// void app_main(void)
// {
//     //create a queue to handle gpio event from isr
//     gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
//     /* 初始化GPIO */
//     gpio_init();

//     printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
//     //start led task
//     xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);
// }

