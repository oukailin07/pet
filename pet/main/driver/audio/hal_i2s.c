#include "hal_i2s.h"
#include "esp_codec_dev_defaults.h"
#include "audio_player.h"
#include "esp_log.h"
#include "string.h"
static const char *TAG = "ADUIO";


static QueueHandle_t audio_path_queue;
const audio_codec_data_if_t *i2s_data_if;
i2s_chan_handle_t i2s_tx_chan;
esp_codec_dev_handle_t play_dev_handle;
extern play_state_t play_state;
const audio_codec_data_if_t *hal_i2s_init(i2s_port_t i2s_port, hal_i2s_pin_t pin_config, i2s_chan_handle_t *tx_channel, uint16_t sample_rate)
{
    esp_err_t ret;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_port, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_channel, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT, // MAX98357 支持 16/24/32bit，一般选 16bit
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_LEFT | I2S_STD_SLOT_RIGHT,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, // MAX98357 不需要 MCLK
            .bclk = pin_config.bclk_pin,
            .ws = pin_config.ws_pin,
            .dout = pin_config.dout_pin,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(*tx_channel, &std_cfg);
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(i2s_channel_enable(*tx_channel));

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = i2s_port,
        .rx_handle = NULL,
        .tx_handle = *tx_channel,
    };
    return audio_codec_new_i2s_data(&i2s_cfg);
}

esp_codec_dev_handle_t audio_codec_init(const audio_codec_data_if_t *data_if)
{
    assert(data_if);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = NULL,
        .data_if = data_if,
    };

    return esp_codec_dev_new(&codec_dev_cfg);
}

esp_err_t aduio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

static esp_err_t aduio_app_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    return ESP_OK;
}

esp_err_t aduio_app_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI("AUDIO_WRITE", "Write %d bytes", len);
    if (aduio_write(audio_buffer, len, bytes_written, 1000) != ESP_OK) {
        ret = ESP_FAIL;
    }

    return ret;
}

esp_err_t audio_app_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };
    ret = esp_codec_dev_close(play_dev_handle);
    ret = esp_codec_dev_open(play_dev_handle, &fs);
    return ret;
}

esp_err_t audio_app_player_music(char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    return audio_player_play(fp);
}

static void audio_app_callback(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event) {
    case 0: /**< Player is idle, not playing audio */
        ESP_LOGI(TAG, "IDLE");
        play_state = PLAY_STATE_PAUSED;
        break;
    case 1:
        ESP_LOGI(TAG, "NEXT");
        break;
    case 2:
        ESP_LOGI(TAG, "PLAYING");
        play_state = PLAY_STATE_PLAYING;
        break;
    case 3:
        ESP_LOGI(TAG, "PAUSE");
        play_state = PLAY_STATE_PAUSED;
        break;
    case 4:
        ESP_LOGI(TAG, "SHUTDOWN");
        break;
    case 5:
        ESP_LOGI(TAG, "UNKNOWN FILE");
        break;
    case 6:
        ESP_LOGI(TAG, "UNKNOWN");
        break;
    }
}

esp_err_t audio_app_player_init(i2s_port_t i2s_port, hal_i2s_pin_t pin_cfg, uint16_t sample_rate)
{
    i2s_data_if = hal_i2s_init(i2s_port, pin_cfg, &i2s_tx_chan, sample_rate);
    if (i2s_data_if == NULL) {
        return ESP_FAIL;
    }

    play_dev_handle = audio_codec_init(i2s_data_if);
    if (play_dev_handle == NULL) {
        return ESP_FAIL;
    }

    esp_codec_dev_set_out_vol(play_dev_handle, 100); // Set volume

    audio_player_config_t config = {
        .mute_fn = aduio_app_mute_function,
        .write_fn = aduio_app_write,
        .clk_set_fn = audio_app_reconfig_clk,
        .priority = 5,
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_app_callback, NULL);


    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_10);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_10, 1); //打开喇叭

    audio_path_queue = xQueueCreate(AUDIO_QUEUE_LENGTH, AUDIO_PATH_MAX_LEN);
    if (!audio_path_queue) {
        ESP_LOGE("AUDIO", "创建路径队列失败");
        return ESP_OK;
    }

    xTaskCreatePinnedToCore(audio_play_task, "audio_play_task", 4096, NULL, 6, NULL,0);
    return ESP_OK;
}


esp_err_t audio_app_player_music_queue(const char *path)
{
    if (!path || strlen(path) >= AUDIO_PATH_MAX_LEN) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueSend(audio_path_queue, path, portMAX_DELAY) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void audio_play_task(void *arg)
{
    char path_buf[AUDIO_PATH_MAX_LEN];

    while (1) {
        if (play_state == PLAY_STATE_PAUSED || play_state == PLAY_STATE_IDLE || play_state == PLAY_STATE_STOPPED) {
            // 获取下一个音频路径
            if (xQueueReceive(audio_path_queue, path_buf, portMAX_DELAY) == pdPASS) {
                audio_app_player_music(path_buf);
            }
        } else {
            // 正在播放中，延迟一下再检查
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}