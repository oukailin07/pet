#ifndef AUDIO_H
#define AUDIO_H

// Audio driver function declarations go here

#include "esp_log.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "driver/spi_master.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_check.h"
#include "driver/i2c_master.h"

#include "driver/i2s_common.h"
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "wav_formate.h"

#include "driver/i2s_std.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_heap_caps.h"
/* Example configurations */
#define EXAMPLE_RECV_BUF_SIZE   (2400)
#define EXAMPLE_SAMPLE_RATE     (16000)
#define EXAMPLE_MCLK_MULTIPLE   (384) // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ    (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME    (70)

#define SPEAKER_I2S_NUM     I2S_NUM_1
#define SPEAKER_I2S_BCK_IO      (GPIO_NUM_14)
#define SPEAKER_I2S_WS_IO       (GPIO_NUM_12)
#define SPEAKER_I2S_DO_IO       (GPIO_NUM_11)
#define SPEAKER_I2S_DI_IO       (-1) // Not used
#define I2S_SPEAKER_SAMPLE_RATE 16000
#define I2S_SPEAKER_BITS_PER_SAMPLE I2S_DATA_BIT_WIDTH_16BIT

#define I2S_MIC_NUM I2S_NUM_0
#define I2S_MIC_SAMPLE_RATE 16000
#define I2S_MIC_BITS_PER_SAMPLE I2S_DATA_BIT_WIDTH_16BIT
#define I2S_MIC_CHANNEL_FORMAT I2S_CHANNEL_FMT_ONLY_LEFT
#define I2S_MIC_BCK_IO GPIO_NUM_9
#define I2S_MIC_WS_IO GPIO_NUM_13
#define I2S_MIC_DI_IO GPIO_NUM_21

typedef struct {
    uint16_t sample_rate;
    uint16_t bits_per_sample;
    gpio_num_t ws_pin;
    gpio_num_t bclk_pin;
    gpio_num_t din_pin;
    i2s_port_t i2s_num;
} i2s_microphone_config_t;

typedef struct {
    i2s_microphone_config_t i2s_config; // i2s的配置信息
    int byte_rate;                      // 1s下的采样数据
    int bytes_all;                      // 录音时间下的所有数据大小
    int sample_size;                    // 每一次采样的大小
    int flash_wr_size;                  // 当前录音的大小
    size_t read_size;                   // i2s读出的长度
} record_info_t;

extern i2s_chan_handle_t rx_handle;

esp_err_t hal_i2s_microphone_init(i2s_microphone_config_t config);
void hal_i2s_record(char *file_path, int record_time);

void i2s_speaker_init();
esp_err_t hal_i2s_get_data(int16_t *buffer, int buffer_len);
#endif // AUDIO_H