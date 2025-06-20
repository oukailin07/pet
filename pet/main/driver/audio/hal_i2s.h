#pragma once

#include "driver/i2s_tdm.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"

typedef struct {
    gpio_num_t bclk_pin;
    gpio_num_t ws_pin;
    gpio_num_t dout_pin;
} hal_i2s_pin_t;

typedef enum {
    PLAY_STATE_IDLE,
    PLAY_STATE_PLAYING,
    PLAY_STATE_PAUSED,
    PLAY_STATE_STOPPED
} play_state_t;
#define AUDIO_QUEUE_LENGTH 10
#define AUDIO_PATH_MAX_LEN 128
esp_err_t audio_app_player_music(char *file_path);
esp_err_t audio_app_player_init(i2s_port_t i2s_port, hal_i2s_pin_t pin_cfg, uint16_t sample_rate);
esp_err_t audio_app_player_music_queue(const char *path);
void audio_play_task(void *arg);