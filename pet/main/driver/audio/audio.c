#include "audio.h"
#include "audio_player.h"
#include "driver/gpio.h"
#include "file_iterator.h"
static const char *TAG = "audio";


i2s_chan_handle_t tx_handle = NULL; // 发送通道句柄

void i2s_speaker_init() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(SPEAKER_I2S_NUM, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_handle, NULL); // 只初始化发送通道

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = I2S_SPEAKER_SAMPLE_RATE, 
            .clk_src = I2S_CLK_SRC_DEFAULT, // 默认时钟源
            .mclk_multiple = I2S_MCLK_MULTIPLE_384, // MCLK 倍频系数
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_SPEAKER_BITS_PER_SAMPLE, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = SPEAKER_I2S_BCK_IO,
            .ws = SPEAKER_I2S_WS_IO,
            .dout = SPEAKER_I2S_DO_IO,
            .din = SPEAKER_I2S_DI_IO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; // 修改为左声道
    std_cfg.slot_cfg.bit_order_lsb = false;          // 低位先行,这边我不确定,但采集的数据确实受环境声音的改变而改变,高位先行却没有
 
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_enable(tx_handle);
}


i2s_chan_handle_t rx_handle = NULL;
record_info_t record_info = {};

esp_err_t hal_i2s_microphone_init(i2s_microphone_config_t config)
{
    esp_err_t ret_val = ESP_OK;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config.i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config.sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(config.bits_per_sample, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_NC,
            .bclk = config.bclk_pin,
            .ws = config.ws_pin,
            .dout = GPIO_NUM_NC,
            .din = config.din_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
    record_info.i2s_config = config;
    return ret_val;
}

esp_err_t hal_i2s_get_data(int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_FAIL;
    size_t bytes_read;
    int audio_chunksize = buffer_len / sizeof(int32_t);
    ret = i2s_channel_read(rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);

    int32_t *tmp_buff = (int32_t *)buffer;
    for (int i = 0; i < audio_chunksize; i++) {
        tmp_buff[i] = tmp_buff[i] >> 14; // 32:8为有效位， 8:0为低8位， 全为0， AFE的输入为16位语音数据，拿29：13位是为了对语音信号放大。
    }

    return ret;
}

void hal_i2s_record(char *file_path, int record_time)
{
    ESP_LOGI(TAG, "Start Record");
    record_info.flash_wr_size = 0;
    record_info.byte_rate = 1 * record_info.i2s_config.sample_rate * record_info.i2s_config.bits_per_sample / 8; // 声道数×采样频率×每样本的数据位数/8。播放软件利用此值可以估计缓冲区的大小。
    record_info.bytes_all = record_info.byte_rate * record_time;                                                 // 设定时间下的所有数据大小
    record_info.sample_size = record_info.i2s_config.bits_per_sample * 1024;                                     // 每一次采样的带下
    const wav_header_t wav_header = WAV_HEADER_PCM_DEFAULT(record_info.bytes_all, record_info.i2s_config.bits_per_sample, record_info.i2s_config.sample_rate, 1);

    // 判断文件是否存在
    struct stat st;
    if (stat(file_path, &st) == 0) {
        ESP_LOGI(TAG, "%s exit", file_path);
        unlink(file_path); // 如果存在就删除
    }

    // 创建WAV文件
    FILE *f = fopen(file_path, "a");
    if (f == NULL) {
        ESP_LOGI(TAG, "Failed to open file");
        return;
    }
    fwrite(&wav_header, sizeof(wav_header), 1, f);

    while (record_info.flash_wr_size < record_info.bytes_all) {
        char *i2s_raw_buffer = heap_caps_calloc(1, record_info.sample_size, MALLOC_CAP_SPIRAM);
        if (i2s_raw_buffer == NULL) {
            continue;
        }

        // Malloc success
        if (i2s_channel_read(rx_handle, i2s_raw_buffer, record_info.sample_size, &record_info.read_size, 100) == ESP_OK) {
            fwrite(i2s_raw_buffer, record_info.read_size, 1, f);
            record_info.flash_wr_size += record_info.read_size;
        } else {
            ESP_LOGI(TAG, "Read Failed!\n");
        }
        free(i2s_raw_buffer);
    }

    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(TAG, "File written on SDCard");
}
