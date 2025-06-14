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
#include "motor.h"



void app_main(void)
{
    nvs_flash_init();
    //initialise_weight_sensor(); //使用HX711传感器
    motor_init();
    motor_control(MOTOR_FORWARD);
    while (1)
    {
        
        /* code */
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
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