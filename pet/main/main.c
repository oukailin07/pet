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


#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hx711.h"

#define AVG_SAMPLES   10
#define GPIO_DATA   GPIO_NUM_4
#define GPIO_SCLK   GPIO_NUM_5

static const char* TAG = "HX711_TEST";

static void weight_reading_task(void* arg);
static void initialise_weight_sensor(void);

static void weight_reading_task(void* arg)
{
    HX711_init(GPIO_DATA, GPIO_SCLK, eGAIN_128); 

    // 第一步：清零（无物体）
    HX711_tare();
    ESP_LOGI(TAG, "完成去皮，请放上一个已知重量的物体（如500g水）...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // 等你手动放上去

    // 第二步：读数并设置校准比例
    unsigned long raw = HX711_get_value(10);  // 获取校准时的读数
    float known_weight_g = 180.0f; // 例如你放的是一瓶500g矿泉水
    float scale = raw / known_weight_g;
    HX711_set_scale(scale);
    ESP_LOGI(TAG, "设置scale = %.2f", scale);

    // 循环读取重量（单位：克）
    float weight;
    while (1)
    {
        weight = HX711_get_units(AVG_SAMPLES);
        ESP_LOGI(TAG, "******* weight = %.2fg *********", weight);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void initialise_weight_sensor(void)
{
    ESP_LOGI(TAG, "****************** Initialing weight sensor **********");
    xTaskCreatePinnedToCore(weight_reading_task, "weight_reading_task", 4096, NULL, 1, NULL,0);
}

void app_main(void)
{
    nvs_flash_init();
    initialise_weight_sensor();
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