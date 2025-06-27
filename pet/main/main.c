#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include <netdb.h>
#include <sys/socket.h>
#include "webserver.h"
#include "esp_codec_dev.h"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "audio.h"
#include "app_spiffs.h"
#include "esp_err.h"
#include "mqtt_client.h"
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "mbedtls/md.h"
#include "esp_sntp.h"

#include "WIFI/ConnectWIFI.h"
#include "wifi_status_manager.h"
#include "my_dns_server.h"
#include "hx711.h"
#include "motor.h"
#include "ws2812.h"
#include "hal_i2s.h"
#include "app_sr.h"
#include "ConnectWIFI.h"
#include "http_tts.h"
#include "key.h"
#include "my_mqtt_client.h"
#include "time_utils.h"
#include "feeding_manager.h"
#include "device_manager.h"
#include "app_timer.h"
#include "websocket_client.h"
#define PCM_FILE_PATH "/spiffs/test.pcm"
#define BUFFER_SIZE 512

play_state_t play_state = PLAY_STATE_PAUSED; // 播放状态
char ip_address[16] = {0}; // 用于存储IP地址

hal_i2s_pin_t hal_i2s_pin = {
    .bclk_pin = GPIO_NUM_14,
    .dout_pin = GPIO_NUM_11,
    .ws_pin = GPIO_NUM_12,
};

// hal_i2s_pin_t hal_i2s_pin = {
//     .bclk_pin = GPIO_NUM_2,
//     .dout_pin = GPIO_NUM_1,
//     .ws_pin = GPIO_NUM_38,
// };
/*
    测试语音唤醒是否正常
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mbedtls/md.h"
#include "cJSON.h"
#include "wifi_status_manager.h"   // 新增
#define TAG "TUYA_MQTT"

// WiFi状态变化回调函数
static void wifi_status_callback(bool connected)
{
    ESP_LOGI("main", "WiFi status changed: %s", connected ? "connected" : "disconnected");
    device_manager_set_wifi_status(connected);
}

// 时间同步函数
void sync_time_sntp(void)
{
    // 设置时区为北京时间 (UTC+8)
    setenv("TZ", "CST-8", 1);
    tzset();
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_setservername(1, "ntp1.aliyun.com");  // 备用NTP服务器
    esp_sntp_setservername(2, "pool.ntp.org");     // 备用NTP服务器
    esp_sntp_init();

    // 等待时间同步完成
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;  // 增加重试次数
    
    ESP_LOGI("SNTP", "开始时间同步...");
    
    while (timeinfo.tm_year < (2023 - 1900) && ++retry < retry_count) {
        ESP_LOGI("SNTP", "等待时间同步... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));  // 等待2秒
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2023 - 1900)) {
        ESP_LOGI("SNTP", "时间同步成功!");
        ESP_LOGI("SNTP", "当前时间: %04d-%02d-%02d %02d:%02d:%02d", 
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGE("SNTP", "时间同步失败，使用系统默认时间");
        // 设置一个默认时间（2024年1月1日 00:00:00）
        struct timeval tv = {
            .tv_sec = 1704067200,  // 2024-01-01 00:00:00 UTC
            .tv_usec = 0
        };
        settimeofday(&tv, NULL);
    }
}

// 自动喂食计划定时任务
void feeding_plan_task(void *arg) {
    while (1) {
        ESP_LOGI("main", "自动喂食计划定时任务");
        feeding_plan_check_and_execute();
        vTaskDelay(pdMS_TO_TICKS(1000)); // 每秒检查一次
    }
}

void app_main(void)
{
    // 1. 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    bsp_spiffs_mount();
    key_init();
    motor_init();
    initialise_weight_sensor();
    
    // 初始化LED灯带
    led_strip_handle_t led_strip = configure_led();
    // 设置LED为蓝色表示系统启动
    led_strip_set_pixel(led_strip, 0, 0, 0, 255);
    led_strip_refresh(led_strip);
    
    ESP_ERROR_CHECK(audio_app_player_init(I2S_NUM_1, hal_i2s_pin, 16 * 1000));
        i2s_microphone_config_t mic_config = {
        .sample_rate = I2S_MIC_SAMPLE_RATE,
        .bits_per_sample = I2S_MIC_BITS_PER_SAMPLE,
        .ws_pin = I2S_MIC_WS_IO,
        .bclk_pin = I2S_MIC_BCK_IO,
        .din_pin = I2S_MIC_DI_IO,
        .i2s_num = I2S_MIC_NUM,
    };
    ret = hal_i2s_microphone_init(mic_config);
    if (ret != ESP_OK) {
        ESP_LOGE("main", "Failed to initialize I2S microphone: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI("main", "I2S microphone initialized successfully");
    app_sr_init();
    app_sr_task_start();
    
    // 初始化设备管理器
    if (device_manager_init() == ESP_OK) {
        ESP_LOGI("main", "设备管理器初始化成功");
    } else {
        ESP_LOGE("main", "设备管理器初始化失败");
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));    
    // 统一启动WiFi状态管理器并连接WiFi
    if (wifi_status_manager_start("adol-3466", "12345678", wifi_status_callback) == ESP_OK) {
    // if (wifi_status_manager_start("CMCC-CMRk", "15221655636", wifi_status_callback) == ESP_OK) {
        ESP_LOGI("main", "WiFi状态管理器启动并连接成功");
    } else {
        ESP_LOGE("main", "WiFi状态管理器启动或连接失败");
    }
    
    // 时间同步
    sync_time_sntp();
    
    // 时间工具使用示例
    vTaskDelay(pdMS_TO_TICKS(2000)); // 等待2秒确保时间同步完成
    
    // 获取并打印当前时间
    time_info_t time_info;
    if (get_current_time_info(&time_info) == ESP_OK) {
        ESP_LOGI("main", "系统启动时间: %d年%d月%d日 %02d:%02d:%02d", 
                 time_info.year, time_info.month, time_info.day,
                 time_info.hour, time_info.minute, time_info.second);
        
        // 获取格式化的时间字符串
        char time_str[64];
        if (get_formatted_time_string(time_str, sizeof(time_str), 0) == ESP_OK) {
            ESP_LOGI("main", "格式化时间: %s", time_str);
        }
        
        // 检查是否为工作时间
        if (is_work_time()) {
            ESP_LOGI("main", "现在是工作时间");
        } else {
            ESP_LOGI("main", "现在不是工作时间");
        }
    } else {
        ESP_LOGE("main", "获取时间信息失败");
    }
    
    // 初始化喂食管理系统
    if (feeding_manager_init() == ESP_OK) {
        ESP_LOGI("main", "喂食管理系统初始化成功");
        
        // 设置LED为绿色表示系统就绪
        led_strip_set_pixel(led_strip, 0, 0, 255, 0);
        led_strip_refresh(led_strip);
        
        // 添加一些示例喂食计划（可选）
        // add_feeding_plan(1, 8, 0, 50.0);   // 星期一 8:00 喂食50g
        // add_feeding_plan(1, 18, 0, 50.0);  // 星期一 18:00 喂食50g
        // add_feeding_plan(2, 8, 0, 50.0);   // 星期二 8:00 喂食50g
        // add_feeding_plan(2, 18, 0, 50.0);  // 星期二 18:00 喂食50g
    } else {
        ESP_LOGE("main", "喂食管理系统初始化失败");
        // 设置LED为红色表示错误
        led_strip_set_pixel(led_strip, 0, 255, 0, 0);
        led_strip_refresh(led_strip);
    }
    
    // 打印设备信息
    const char* device_id = device_manager_get_device_id();
    if (device_id && device_id[0] != '\0') {
        ESP_LOGI("main", "设备ID: %s", device_id);
    } else {
        ESP_LOGI("main", "设备ID: 未初始化，等待服务器分配");
    }
    
    //tuya_mqtt_start();
    // 启动WebSocket客户端
    websocket_client_start();

    // 启动自动喂食计划定时任务
    xTaskCreatePinnedToCore(feeding_plan_task, "feeding_plan_task", 4096, NULL, 5, NULL,1);
}












// void app_main(void)
// {
//     ESP_ERROR_CHECK( nvs_flash_init() );
//     NvsWriteDataToFlash("","","");
//     char WIFI_Name[50] = { 0 };
//     char WIFI_PassWord[50] = { 0 }; /*读取保存的WIFI信息*/
//     if(NvsReadDataFromFlash("WIFI Config Is OK!",WIFI_Name,WIFI_PassWord) == 0x00)
//     {
//         printf("WIFI SSID     :%s\r\n",WIFI_Name    );
//         printf("WIFI PASSWORD :%s\r\n",WIFI_PassWord);
//         printf("开始初始化WIFI Station 模式\r\n");
//         wifi_init_sta(WIFI_Name,WIFI_PassWord);         /*按照读取的信息初始化WIFI Station模式*/
        
//     }
//     else
//     {
//         printf("未读取到WIFI配置信息\r\n");
//         printf("开始初始化WIFI AP 模式\r\n");
//         wifi_init_softap_sta();
//         //WIFI_AP_Init();     /*上电后配置WIFI为AP模式*/
//         //vTaskDelay(1000 / portTICK_PERIOD_MS);
//         dns_server_start();  //开启DNS服务
//         web_server_start();  //开启http服务
//     }
// }