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
static const char *TAG = "main";
#define PCM_FILE_PATH "/spiffs/test.pcm"
#define BUFFER_SIZE 512

#define I2C_PORT    0
#define I2C_SDA_IO  5
#define I2C_SCL_IO  4
#define I2C_FREQ_HZ 100000


// void app_main(void)
// {
//     // 配置 I2C 总线
//     i2c_master_bus_config_t bus_config = {
//         .clk_source = I2C_CLK_SRC_DEFAULT,
//         .i2c_port = I2C_PORT,
//         .scl_io_num = I2C_SCL_IO,
//         .sda_io_num = I2C_SDA_IO,
//         .glitch_ignore_cnt = 7,
//         .flags.enable_internal_pullup = true,
//     };

//     i2c_master_bus_handle_t bus_handle;
//     ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

//     ESP_LOGI(TAG, "Scanning I2C bus...");

//     for (uint8_t addr = 0x03; addr < 0x78; addr++) {
//         esp_err_t ret = i2c_master_probe(bus_handle, addr, 50);  // ✅ 正确传入地址

//         if (ret == ESP_OK) {
//             ESP_LOGI(TAG, "Found I2C device at address 0x%02X", addr);
//         }
//     }

//     ESP_LOGI(TAG, "Scan complete.");
// }
void app_main(void)
{
    nvs_flash_init();
    bsp_i2c_init();

    app_spiffs_init();
    //initialise_weight_sensor(); //使用HX711传感器
    motor_init();
    motor_control(MOTOR_FORWARD);
    esp_codec_dev_handle_t codec_dev = bsp_audio_codec_speaker_microphone_init();
    //led_strip_handle_t led_strip = configure_led();
    ESP_LOGI(TAG, "Opening PCM file...");
    FILE *fp = fopen(PCM_FILE_PATH, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open PCM file");
        return;
    }
        // 打开播放参数
    esp_codec_dev_sample_info_t fs_cfg = {
        .sample_rate = 8000,
        .channel = 1,    // 单声道
    };
    ESP_ERROR_CHECK(esp_codec_dev_open(codec_dev, &fs_cfg));
    uint8_t buf[512];
    size_t bytes_read;

    ESP_LOGI(TAG, "Starting PCM playback");
    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0) {
        esp_codec_dev_write(codec_dev, buf, bytes_read);

    }

    ESP_LOGI(TAG, "Playback finished");
    fclose(fp);
    esp_codec_dev_close(codec_dev);
    
}

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