#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"


#include "key.h"
#define GPIO0_INPUT_IO     0
#define DEBOUNCE_TIME_MS   30
#define DOUBLE_CLICK_INTERVAL_MS 300
#define LONG_PRESS_TIME_MS 3000

typedef enum {
    BUTTON_EVENT_SINGLE_CLICK,
    BUTTON_EVENT_DOUBLE_CLICK,
    BUTTON_EVENT_LONG_PRESS
} button_event_t;

static QueueHandle_t gpio_evt_queue = NULL;

typedef struct {
    int64_t last_press_time;
    int64_t last_release_time;
    int click_count;
    bool is_pressed;
    bool long_press_reported;
} button_state_t;

static button_state_t btn_state = {0};

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void button_event_callback(button_event_t event)
{
    switch (event) {
        case BUTTON_EVENT_SINGLE_CLICK:
            printf("==> 单击切换模式\n");
            break;
        case BUTTON_EVENT_DOUBLE_CLICK:
            printf("==> 三击显示ip或者进入配网\n");
            break;
        case BUTTON_EVENT_LONG_PRESS:
            printf("==> 长按清空保存在flash里面的wifi信息\n");
            break;
    }
}

static void led_task(void* arg)
{
    uint32_t io_num;
    int last_level = 1; // 默认未按下
    for(;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS)); // 消抖延迟
            int level = gpio_get_level(io_num);
            int64_t now = esp_timer_get_time() / 1000; // 当前时间（毫秒）

            if (level == 0 && last_level == 1) { // 按下
                btn_state.is_pressed = true;
                btn_state.last_press_time = now;
                btn_state.long_press_reported = false;
            } else if (level == 1 && last_level == 0) { // 松开
                btn_state.is_pressed = false;
                btn_state.last_release_time = now;

                int press_duration = now - btn_state.last_press_time;
                if (press_duration >= LONG_PRESS_TIME_MS) {
                    if (!btn_state.long_press_reported) {
                        button_event_callback(BUTTON_EVENT_LONG_PRESS);
                        btn_state.long_press_reported = true;
                    }
                } else {
                    btn_state.click_count++;
                    // 延迟检测双击
                    vTaskDelay(pdMS_TO_TICKS(DOUBLE_CLICK_INTERVAL_MS));
                    if (btn_state.click_count == 1) {
                        button_event_callback(BUTTON_EVENT_SINGLE_CLICK);
                    } else if (btn_state.click_count == 3) {
                        button_event_callback(BUTTON_EVENT_DOUBLE_CLICK);
                    }
                    btn_state.click_count = 0;
                }
            }

            last_level = level;
        }
    }
}
void gpio_init(void)
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
  
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO0_INPUT_IO, GPIO_INTR_LOW_LEVEL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO0_INPUT_IO, gpio_isr_handler, (void*) GPIO0_INPUT_IO);

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

