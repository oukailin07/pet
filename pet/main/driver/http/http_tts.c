#include "http_tts.h"
#include "esp_log.h"
#include "audio.h"
#include "esp_crt_bundle.h"  // 包含根证书包
esp_http_client_handle_t client;
char *url = "https://tsn.baidu.com/text2audio";
char *access_token = "24.5ba86526d0e457df7fdb3222e76a23e7.2592000.1752981760.282335-119296490";
char *formate = "tex=%s&tok=%s&cuid=RKn4rgXEGbCy08SazzcZlufQqy9mfsu9&ctp=1&lan=zh&spd=5&pit=5&vol=5&per=4&aue=4"; // PCM 16K
char *text = "在的,主人";
size_t text_url_encode_size = 0;

static const char *TAG = "HTTP_BAIDU";

extern i2s_chan_handle_t i2s_tx_chan;
esp_err_t app_http_baidu_tts_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "Received length:%d", evt->data_len);

        // 将百度 TTS 的 PCM 单声道转为双声道
        int mono_samples = evt->data_len / 2; // 每个采样 2 字节
        int stereo_len = mono_samples * 2 * 2; // 每个样本*2通道*2字节

        int16_t *mono_data = (int16_t *)evt->data;
        int16_t *stereo_data = heap_caps_malloc(stereo_len, MALLOC_CAP_DMA);
        if (stereo_data == NULL) {
            ESP_LOGE(TAG, "Failed to allocate stereo buffer");
            return ESP_FAIL;
        }

        for (int i = 0; i < mono_samples; ++i) {
            stereo_data[2 * i] = mono_data[i];       // 左声道
            stereo_data[2 * i + 1] = mono_data[i];   // 右声道
        }

        size_t bytes_written = 0;
        i2s_channel_write(i2s_tx_chan, stereo_data, stereo_len, &bytes_written, 100);
        free(stereo_data);
    }

    return ESP_OK;
}

void http_get_ip_tts(char *ip)
{
    ESP_LOGI(TAG, "HTTP TTS get IP: %s", ip);
    // Http
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_baidu_tts_event_handler,
        .buffer_size = 20 * 1024,  // 20 KB
        .buffer_size_tx = 20 * 1024,  // 20 KB 发送缓冲
        .timeout_ms = 30000, // 设置超时时间为 10 秒
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        
    };

    config.url = url;
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "Accept", "*/*");

    url_encode((unsigned char *)ip, strlen(ip), &text_url_encode_size, NULL, 0);

    ESP_LOGI(TAG, "text size after url:%zu", text_url_encode_size);

    char *text_url_encode = heap_caps_calloc(1, text_url_encode_size + 1, MALLOC_CAP_DMA);

    if (text_url_encode == NULL) {
        ESP_LOGI(TAG, "Malloc text url encode failed");
        return;
    }

    url_encode((unsigned char *)ip, strlen(ip), &text_url_encode_size, (unsigned char *)text_url_encode, text_url_encode_size + 1);

    char *payload = heap_caps_calloc(1, strlen(formate) + strlen(access_token) + strlen(text_url_encode) + 1, MALLOC_CAP_DMA);

    if (payload == NULL) {
        free(text_url_encode);
        ESP_LOGI(TAG, "Malloc payload failed");
        return;
    }

    sprintf(payload, formate, text_url_encode, access_token);
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    free(text_url_encode);
    free(payload);
}