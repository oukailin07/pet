#include <stdio.h>
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
 
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
 
#include <netdb.h>
#include <sys/socket.h>
 
#include "WIFI/ConnectWIFI.h"
#include "my_dns_server.h"
 
#include "webserver.h"
#include "esp_codec_dev.h"

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hx711.h"
#include "motor.h"
#include "ws2812.h"
#include "audio.h"
#include "app_spiffs.h"
#include "esp_err.h"
#include "hal_i2s.h"
#include "app_sr.h"
static const char *TAG = "main";
#define PCM_FILE_PATH "/spiffs/test.pcm"
#define BUFFER_SIZE 512

/*
    测试语音唤醒是否正常
*/
// void app_main(void)
// {
//     nvs_flash_init();
//     i2s_speaker_init();
//         i2s_microphone_config_t mic_config = {
//         .sample_rate = I2S_MIC_SAMPLE_RATE,
//         .bits_per_sample = I2S_MIC_BITS_PER_SAMPLE,
//         .ws_pin = I2S_MIC_WS_IO,
//         .bclk_pin = I2S_MIC_BCK_IO,
//         .din_pin = I2S_MIC_DI_IO,
//         .i2s_num = I2S_MIC_NUM,
//     };
//     esp_err_t ret = hal_i2s_microphone_init(mic_config);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to initialize I2S microphone: %s", esp_err_to_name(ret));
//         return;
//     }
//     ESP_LOGI(TAG, "I2S microphone initialized successfully");
//     app_sr_init();
//     app_sr_task_start();
// }


hal_i2s_pin_t hal_i2s_pin = {
    .bclk_pin = GPIO_NUM_14,
    .dout_pin = GPIO_NUM_11,
    .ws_pin = GPIO_NUM_12,
};
/*
    测试喇叭是否正常
*/
void app_main(void)
{
    nvs_flash_init();
    bsp_spiffs_mount();

    ESP_ERROR_CHECK(audio_app_player_init(I2S_NUM_1, hal_i2s_pin, 16 * 1000));
    ESP_ERROR_CHECK(audio_app_player_music("/spiffs/new_epic.mp3"));
}


/*
    测试称重传感器 ws2812 电机 是否正常
*/
// void app_main(void)
// {
//     nvs_flash_init();
//     //initialise_weight_sensor(); //使用HX711传感器
//     motor_init();
//     motor_control(MOTOR_FORWARD);
//     led_strip_handle_t led_strip = configure_led();
//     while (1)
//     {
//         led_strip_set_pixel(led_strip, 0, 0, 255, 0);
//         led_strip_refresh(led_strip);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//         led_strip_set_pixel(led_strip, 0, 0, 0, 255); // Set LED to blue
//         led_strip_refresh(led_strip);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
    
// }

 
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