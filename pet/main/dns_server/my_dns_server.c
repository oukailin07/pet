#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
 
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "mbedtls/md5.h"
#include "webserver.h"
 
static const char *TAG = "DNS_SERVER";
 
static int create_udp_socket(int port)
{
    struct sockaddr_in saddr = { 0 };
    int sock = -1;
    int err = 0;
 
    sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG,"Failed to create socket. Error %d", errno);
        return -1;
    }
 
    // Bind the socket to any address
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    err = bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (err < 0) {
        ESP_LOGE(TAG,"Failed to bind socket. Error %d", errno);
        goto err;
    }
 
    // All set, socket is configured for sending and receiving
    return sock;
 
err:
    close(sock);
    return -1;
}
 
static void my_dns_server(void *pvParameters)
{
    uint8_t data[512];  // DNS 最大包长 512
    int len = 0;
    struct sockaddr_in client = { 0 };
    socklen_t client_len = sizeof(struct sockaddr_in);

    ESP_LOGI(TAG, "DNS server start ...");

    int sock = create_udp_socket(53);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create UDP socket for DNS");
        vTaskDelete(NULL);
        return;
    }

    const uint8_t redirect_ip[4] = {192, 168, 4, 1};  // 统一回应的 IP

    while (1) {
        len = recvfrom(sock, data, sizeof(data), 0, (struct sockaddr *)&client, &client_len);
        if (len < 0 || len > sizeof(data)) {
            ESP_LOGW(TAG, "recvfrom error or oversized packet: %d", len);
            continue;
        }

        ESP_LOGI(TAG, "Received DNS query from %s:%d",
                 inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        // 打印 DNS 请求域名（解析压缩字段复杂，简单打印）
        printf("DNS query raw name: ");
        for (int i = 12; i < len; ++i) {
            if ((data[i] >= 32) && (data[i] <= 126)) putchar(data[i]);
            else putchar('.');
        }
        putchar('\n');

        // 构建回应
        // 设置为响应包
        data[2] |= 0x80; // 标志位，设置为响应
        data[3] |= 0x80; // 递归可用
        data[7] = 1;     // Answer RRs = 1

        int answer_offset = len;

        // 回答部分：指针指向查询名称
        data[len++] = 0xC0; data[len++] = 0x0C;       // Name pointer
        data[len++] = 0x00; data[len++] = 0x01;       // Type A
        data[len++] = 0x00; data[len++] = 0x01;       // Class IN
        data[len++] = 0x00; data[len++] = 0x00; data[len++] = 0x00; data[len++] = 0x3C; // TTL 60s
        data[len++] = 0x00; data[len++] = 0x04;       // Data length = 4
        memcpy(&data[len], redirect_ip, 4); len += 4; // IP address

        int sent = sendto(sock, data, len, 0, (struct sockaddr *)&client, client_len);
        if (sent < 0) {
            ESP_LOGE(TAG, "Failed to send DNS response");
        } else {
            ESP_LOGI(TAG, "DNS response sent to %s:%d", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGI(TAG, "DNS server stop");
    shutdown(sock, 0);
    close(sock);
    vTaskDelete(NULL);
}

 
void dns_server_start()
{
    xTaskCreate(&my_dns_server, "dns_task", 2048*4, NULL, 5, NULL);
}