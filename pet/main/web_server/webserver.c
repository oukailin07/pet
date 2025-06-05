#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
 
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
 
#include "nvs_flash.h"
 
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
 
#include "esp_http_server.h"
 
#include "webserver.h"
#include "WIFI/ConnectWIFI.h"
 
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
 
static const char *TAG = "WEB_SERVER";
 
#define MIN( x, y ) ( ( x ) < ( y ) ? ( x ) : ( y ) )
 
/* Send HTTP response with a run-time generated html consisting of
 * a list of all files and folders under the requested path.
 * In case of SPIFFS this returns empty list when path is any
 * string other than '/', since SPIFFS doesn't support directories */
 static esp_err_t http_SendText_html(httpd_req_t *req)
 {
     httpd_resp_set_type(req, "text/html");
     
     // 发送第一块数据
     httpd_resp_send_chunk(req, (const char *)index_html_start, index_html_end - index_html_start);
     
     // 发送第二块数据
     const char TxBuffer[] = "<h1> SSID1 other WIFI</h1>";
     httpd_resp_send_chunk(req, TxBuffer, strlen(TxBuffer));
     
     // 必须发送空 chunk 表示结束！
     httpd_resp_send_chunk(req, NULL, 0);
     return ESP_OK;
 }
unsigned char CharToNum(unsigned char Data)
{
    if(Data >= '0' && Data <= '9')
    {
        return Data - '0';
    }
    else if(Data >= 'a' && Data <= 'f')
    {
        switch (Data)
        {
            case 'a':return 10;
            case 'b':return 11;
            case 'c':return 12;
            case 'd':return 13;
            case 'e':return 14;
            case 'f':return 15;
        default:
            break;
        }
    }
    else if(Data >= 'A' && Data <= 'F')
    {
        switch (Data)
        {
            case 'A':return 10;
            case 'B':return 11;
            case 'C':return 12;
            case 'D':return 13;
            case 'E':return 14;
            case 'F':return 15;
        default:
            break;
        }
    }
    return 0;
}
/* 强制门户访问时连接wifi后的第一次任意GET请求 */
static esp_err_t HTTP_FirstGet_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"HTTP_FirstGet_handler start");
    http_SendText_html(req);
    ESP_LOGI(TAG,"HTTP_FirstGet_handler end");
    return ESP_OK;
}
/* 门户页面发回的，带有要连接的WIFI的名字和密码 */
static esp_err_t WIFI_Config_POST_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"WIFI_Config_POST_handler start");
    char buf[200] = {0};  // 扩大缓存，避免数据被截断
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf) - 1)); // 预留末尾'\0'
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        buf[ret] = '\0'; // 确保字符串结束
        remaining -= ret;
    }

    ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG, "%s", buf);
    ESP_LOGI(TAG, "====================================");

    // 回显客户端
    char response[256];
    snprintf(response, sizeof(response), "The WIFI To Connect : %s", buf);
    httpd_resp_send_chunk(req, response, strlen(response));
    httpd_resp_send_chunk(req, NULL, 0);  // 发送结束

    char wifi_name[50] = {0};
    char wifi_password[50] = {0};

    // 注意字段名为 "password"，不是 "passWord"
    esp_err_t e1 = httpd_query_key_value(buf, "ssid", wifi_name, sizeof(wifi_name));
    esp_err_t e2 = httpd_query_key_value(buf, "password", wifi_password, sizeof(wifi_password));

    if (e1 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SSID, error=%d", e1);
        return ESP_FAIL;
    }

    if (e2 != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get password, error=%d", e2);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "SSID = %s", wifi_name);
    ESP_LOGI(TAG, "Password = %s", wifi_password);

    // 写入 NVS 并重启
    NvsWriteDataToFlash("WIFI Config Is OK!", wifi_name, wifi_password);
    ESP_LOGI(TAG,"WIFI_Config_POST_handler end");
    esp_restart();

    return ESP_OK;
}


esp_err_t index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"index_html_handler start");
    const char *resp_str =
        "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
        "<title>WiFi配置</title>"
        "<style>"
        "body { font-family: Arial; background-color: #f2f2f2; text-align: center; padding-top: 50px; }"
        "form { background-color: #fff; padding: 20px; border-radius: 10px; display: inline-block; box-shadow: 0 0 10px #aaa; }"
        "input { margin: 10px; padding: 8px; width: 80%; }"
        "input[type='submit'] { width: auto; background-color: #4CAF50; color: white; border: none; cursor: pointer; }"
        "</style>"
        "</head><body>"
        "<h2>欢迎使用 WiFi 配置</h2>"
        "<form action='/configwifi' method='POST'>"
        "<label>SSID:<br><input name='ssid' required></label><br>"
        "<label>密码:<br><input name='password' type='password' required></label><br>"
        "<input type='submit' value='连接'>"
        "</form>"
        "</body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    ESP_LOGI(TAG,"index_html_handler end");
    return ESP_OK;
}

esp_err_t http_redirect_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"http_redirect_handler start");
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");  // 重定向地址
    httpd_resp_send(req, NULL, 0);  // No body
    ESP_LOGI(TAG,"http_redirect_handler end");
    return ESP_OK;
}


// 定义最大扫描数量
#define MAX_APs 20

#include "cJSON.h" // ESP-IDF 已内置

esp_err_t get_wifi_list_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling /get_wifi_list request");

    // 1. 启动WiFi扫描（阻塞模式）
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    // 2. 获取扫描结果
    wifi_ap_record_t ap_info[MAX_APs];
    uint16_t ap_count = MAX_APs;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
    ESP_LOGI(TAG, "Found %d APs", ap_count);

    // 3. 构建JSON响应
    cJSON *root = cJSON_CreateObject();
    cJSON *wifi_array = cJSON_CreateArray();
    for (int i = 0; i < ap_count; i++) {
        if (strlen((char*)ap_info[i].ssid) > 0) {
            cJSON_AddItemToArray(wifi_array, cJSON_CreateString((char*)ap_info[i].ssid));
        }
    }
    cJSON_AddItemToObject(root, "wifi_list", wifi_array);

    // 4. 发送响应
    const char *json_str = cJSON_Print(root);
    ESP_LOGI(TAG, "Sending JSON: %s", json_str);  // 关键日志

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    // 5. 清理
    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}



void web_server_start(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 13;
    config.stack_size = 1024*16;
    config.uri_match_fn = httpd_uri_match_wildcard;
    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server!");
        return;
    }

    // 在 web_server_start() 注册这些路径
    httpd_uri_t captive_portal_urls[] = {
        { .uri = "/generate_204", .method = HTTP_GET, .handler = http_redirect_handler },
        { .uri = "/hotspot-detect.html", .method = HTTP_GET, .handler = http_redirect_handler },
        { .uri = "/library/test/success.html", .method = HTTP_GET, .handler = http_redirect_handler },
        { .uri = "/connectivity-check.html", .method = HTTP_GET, .handler = http_redirect_handler },
    };

    for (int i = 0; i < sizeof(captive_portal_urls)/sizeof(captive_portal_urls[0]); i++) {
        httpd_register_uri_handler(server, &captive_portal_urls[i]);
    }
    httpd_uri_t wifi_list = {
        .uri = "/get_wifi_list",
        .method = HTTP_GET,
        .handler = get_wifi_list_handler,
        .user_ctx  = NULL,
    };
    httpd_register_uri_handler(server, &wifi_list);

    // URI handler for file download
    httpd_uri_t file_download = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = HTTP_FirstGet_handler,
        .user_ctx  = NULL,
    };
    httpd_register_uri_handler(server, &file_download);

    // URI handler for file upload (配置 WiFi)
    httpd_uri_t file_upload = {
        .uri       = "/configwifi",
        .method    = HTTP_POST,
        .handler   = WIFI_Config_POST_handler,
        .user_ctx  = NULL,
    };
    httpd_register_uri_handler(server, &file_upload);




}
