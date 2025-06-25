#include "esp_timer.h"
#include "app_timer.h"
#include "motor.h"
#include "esp_sntp.h"
#define TAG "APP_TIMER"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>
#include <sys/time.h>
// 回调函数定义
static void my_timer_callback(void* arg)
{
    motor_control(MOTOR_STOP);
}

// 创建一次性定时器的函数
void create_one_shot_timer(int seconds)
{
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = &my_timer_callback,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "my_timer"
    };

    esp_err_t ret = esp_timer_create(&timer_args, &timer);
    if (ret != ESP_OK) {
        printf("创建定时器失败\n");
        return;
    }

    esp_timer_start_once(timer, seconds * 1000000ULL); // 转换为微秒
    printf("定时器已启动，%d 秒后触发\n", seconds);
}