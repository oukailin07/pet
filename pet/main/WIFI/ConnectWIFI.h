#ifndef _CONNECT_WIFI_H
#define _CONNECT_WIFI_H
#include <string.h>
#include <stdlib.h>
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
 
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"
 
#include "lwip/err.h"
#include "lwip/sys.h"
 
void WIFI_AP_Init(void);
 
void wifi_init_sta(char *WIFI_Name,char *WIFI_PassWord);
void wifi_init_softap_sta();
void NvsWriteDataToFlash(char *ConfirmString,char *WIFI_Name,char *WIFI_PassWord);
unsigned char NvsReadDataFromFlash(char *ConfirmString,char *WIFI_Name,char *WIFI_PassWord);
 
 
#endif