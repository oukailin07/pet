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
static const char *TAG = "main";
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

#define TAG "TUYA_MQTT"

void sync_time_sntp(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_init();

    // 等待时间同步完成
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2023 - 1900) && ++retry < retry_count) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d)", retry);
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    ESP_LOGI("SNTP", "System time is set to: %lld", (long long)now);
}

void app_main(void)
{
    nvs_flash_init();
    bsp_spiffs_mount();
    key_init();
    motor_init();
    initialise_weight_sensor();
    
    led_strip_handle_t led_strip = configure_led();
    // led_strip_set_pixel(led_strip, 0, 0, 255, 0);
    // led_strip_refresh(led_strip);
    ESP_ERROR_CHECK(audio_app_player_init(I2S_NUM_1, hal_i2s_pin, 16 * 1000));
        i2s_microphone_config_t mic_config = {
        .sample_rate = I2S_MIC_SAMPLE_RATE,
        .bits_per_sample = I2S_MIC_BITS_PER_SAMPLE,
        .ws_pin = I2S_MIC_WS_IO,
        .bclk_pin = I2S_MIC_BCK_IO,
        .din_pin = I2S_MIC_DI_IO,
        .i2s_num = I2S_MIC_NUM,
    };
    esp_err_t ret = hal_i2s_microphone_init(mic_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S microphone: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2S microphone initialized successfully");
    app_sr_init();
    app_sr_task_start();
    wifi_init_sta("adol-3466","12345678");
    //http_get_ip_tts(ip_address);
sync_time_sntp();
    tuya_mqtt_start();

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