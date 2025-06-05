#include <stdio.h>
#include "WIFI/ConnectWIFI.h"
#include <stddef.h>  // 加这个头文件确保 size_t 类型
/* The examples use WiFi configuration that you can set via project configuration menu.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "esp-32"
#define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_WIFI_CHANNEL   (10)
#define EXAMPLE_MAX_STA_CONN       (1)
 
// #define EXAMPLE_ESP_WIFI_SSID      "espConnect"
// #define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
 
#define CONFIG_ESP_WIFI_AUTH_OPEN 1
 
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif
 
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        //ESP_LOGI("ESP32", "station " MACSTR " join, AID=%u", MAC2STR(event->mac), event->aid);
        ESP_LOGI("ESP32","wifi连接\n");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGE("ESP32", "Disconnected, reason: %d", event->reason);
    }
    
    
}
 
void WIFI_AP_Init(void)
{
    ESP_LOGI("ESP32", "WIFI Start Init");
 
    ESP_ERROR_CHECK(esp_netif_init());                  /*初始化底层TCP/IP堆栈*/
    ESP_ERROR_CHECK(esp_event_loop_create_default());   /*创建默认事件循环*/
    esp_netif_create_default_wifi_ap();                 /*创建默认WIFI AP,如果出现任何初始化错误,此API将中止*/
 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));               /*初始化WiFi为WiFi驱动程序分配资源，如WiFi控制结构、RX/TX缓冲区、WiFi NVS结构等。此WiFi还启动WiFi任务。*/
 
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
 
        wifi_config_t wifi_config = {
            .ap = {
                .ssid = EXAMPLE_ESP_WIFI_SSID,
                .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
                .channel = EXAMPLE_ESP_WIFI_CHANNEL,
                .password = EXAMPLE_ESP_WIFI_PASS,
                .max_connection = EXAMPLE_MAX_STA_CONN,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK
            },
        };
        if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }
 
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
 
        ESP_LOGI("ESP32", "wifi_init_softap finished. SSID:%s password:%s channel:%d",
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}
 
 
 
 
 
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
 
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
 
static const char *TAG = "wifi station";
 
static int s_retry_num = 0;
 
 
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
 
 
#define MAX_RETRY 5
static int retry_count = 0;

void wifi_init_sta(char *WIFI_Name, char *WIFI_PassWord)
{
    s_wifi_event_group = xEventGroupCreate();
 
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
 
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
 
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
 
    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. */
            .threshold.authmode = WIFI_AUTH_WPA_PSK,
        },
    };
 
    strcpy((char *)&wifi_config.sta.ssid, WIFI_Name);
    strcpy((char *)&wifi_config.sta.password, WIFI_PassWord);
 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
 
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
 
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_Name, WIFI_PassWord);
        //xTaskCreate(tcp_client_task, "tcp_client", 1024 * 10, NULL, 5, NULL);/*TCP_client 连接TCP*/
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s, retry count: %d",
                 WIFI_Name, WIFI_PassWord, retry_count);
        
        if (retry_count >= MAX_RETRY) {
            ESP_LOGE(TAG, "Max retry count reached, resetting...");
            NvsWriteDataToFlash("","",""); /* 清除保存的连接信息 */
            esp_restart(); /* 或者这里可以选择重启设备 */
        } else {
            retry_count++;
            ESP_LOGI(TAG, "Retrying to connect...");
            wifi_init_sta(WIFI_Name, WIFI_PassWord); /* 递归重试 */
        }
    } 
    else 
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}
/* Initialize wifi station */
esp_netif_t *wifi_init_sta1(void)
{
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = "okl",
            .password = "12345678",
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
            * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    return esp_netif_sta;
}

void wifi_init_softap() {
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "AP mode initialized. SSID: %s", EXAMPLE_ESP_WIFI_SSID);
}

void wifi_init_softap_sta()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // /* Initialize event group */
    // s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    /* Initialize AP */
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();
    /* Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start() );

}

#define WIFI_SSID_MAX_LEN 128
#define WIFI_PASSWORD_MAX_LEN 256
 
void NvsWriteDataToFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord)
{
    nvs_handle handle;
    // 要写入的Wi-Fi信息
    wifi_config_t wifi_config_to_store;

    // 确保Wi-Fi名称和密码不超过最大长度
    if (strlen(WIFI_Name) > WIFI_SSID_MAX_LEN || strlen(WIFI_PassWord) > WIFI_PASSWORD_MAX_LEN) {
        ESP_LOGE(TAG, "SSID or password is too long.");
        return;
    }

    // 将SSID和密码复制到wifi_config_t结构
    strcpy((char *)wifi_config_to_store.sta.ssid, WIFI_Name);
    strcpy((char *)wifi_config_to_store.sta.password, WIFI_PassWord);

    printf("set size:%u\r\n", sizeof(wifi_config_to_store));

    // 打开NVS存储
    ESP_ERROR_CHECK(nvs_open("customer data", NVS_READWRITE, &handle));

    // 存储确认信息
    ESP_ERROR_CHECK(nvs_set_str(handle, "String", ConfirmString));

    // 存储Wi-Fi配置
    ESP_ERROR_CHECK(nvs_set_blob(handle, "blob_wifi", &wifi_config_to_store, sizeof(wifi_config_to_store)));

    // 存储版本信息（如果需要）
    // uint8_t version_for_store[4] = {0x01, 0x01, 0x01, 0x00};
    // ESP_ERROR_CHECK(nvs_set_blob(handle, "blob_version", version_for_store, sizeof(version_for_store)));

    // 提交并关闭NVS
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

 
unsigned char NvsReadDataFromFlash(char *ConfirmString,char *WIFI_Name,char *WIFI_PassWord)
{
    nvs_handle handle;
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA2 = "String";
    static const char *DATA3 = "blob_wifi";
    // static const char *DATA4 = "blob_version";
 
    size_t str_length = 50;
    char str_data[50] = {0};
    wifi_config_t wifi_config_stored;
    // uint8_t version[4] = {0};
    // uint32_t version_len = 4;
    
    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored));
    size_t wifi_len = sizeof(wifi_config_stored);
 
    ESP_ERROR_CHECK( nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle) );
 
    esp_err_t err = nvs_get_str(handle, DATA2, str_data, &str_length);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to read string from NVS: %s", esp_err_to_name(err));
    }
    
    err = nvs_get_blob(handle, DATA3, &wifi_config_stored, &wifi_len);
    if (err != ESP_OK) {
        ESP_LOGE("NVS", "Failed to read blob from NVS: %s", esp_err_to_name(err));
    }
    // ESP_ERROR_CHECK ( nvs_get_blob(handle, DATA4, version, &version_len) );
 
    printf("[data1]: %s len:%u\r\n", str_data, str_length);

    printf("[data3]: ssid:%s passwd:%s\r\n", wifi_config_stored.sta.ssid, wifi_config_stored.sta.password);
 
    strcpy(WIFI_Name,(char *)&wifi_config_stored.sta.ssid);
    strcpy(WIFI_PassWord,(char *)&wifi_config_stored.sta.password);
 
    nvs_close(handle);
 
    if(strcmp(ConfirmString,str_data) == 0)
    {
        return 0x00;
    }
    else
    {
        return 0xFF;
    }
}