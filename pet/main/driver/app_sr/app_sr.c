#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"
#include "app_sr.h"
#include "esp_mn_speech_commands.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
// #include "driver/i2s.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"
#include "string.h"
#include "hal_i2s.h"
#include "string.h"
#include "ws2812.h"
#include "audio.h"
#include "app_timer.h"
#include "motor.h"
#include "http_tts.h"
static const char *TAG = "APP_SR";


esp_afe_sr_data_t *g_afe_data = NULL;
typedef struct {
    wakenet_state_t     wakenet_mode;
    esp_mn_state_t      state;
    int                 command_id;
} sr_result_t;

static QueueHandle_t            g_result_que    = NULL;
int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;
static model_iface_data_t       *model_data     = NULL;
static const esp_mn_iface_t     *multinet       = NULL;
const char *cmd_phoneme[8] = {
    "chu liang",
    "qing chu liang",
    "chu yi fen liang",
    "chu liang fen liang",
    "chu san fen liang",
    "chu si fen liang",
    "chu wu fen liang",
    "she bei di zhi"
};

esp_err_t app_sr_init(void)
{
    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    //////

    afe_config->wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    afe_config->aec_init = false;
    afe_handle = esp_afe_handle_from_config(afe_config);
    g_afe_data = afe_handle->create_from_config(afe_config);
    //////
    char *mn_name = esp_srmodel_filter(models, ESP_MN_CHINESE, NULL);
    if (NULL == mn_name) {
        printf("No multinet model found");
        return ESP_FAIL;
    }
    multinet = esp_mn_handle_from_name(mn_name);
    model_data = multinet->create(mn_name, 5760);//设置唤醒超时时间
    printf( "load multinet:%s", mn_name);
    esp_mn_commands_clear();//清除唤醒指令
    for (int i = 0; i < sizeof(cmd_phoneme) / sizeof(cmd_phoneme[0]); i++) {
        esp_mn_commands_add(i, (char *)cmd_phoneme[i]);//逐个将唤醒指令放入
    }
    esp_mn_commands_update();//更新命令词列表
    esp_mn_commands_print();
    multinet->print_active_speech_commands(model_data);//输出目前激活的命令词
    //////
    afe_config_free(afe_config);
    
    task_flag = 1;
    g_result_que = xQueueCreate(1, sizeof(sr_result_t));
    return ESP_OK;
}

void app_sr_feed_task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;

    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    int16_t *feed_buff = (int16_t *) malloc(feed_chunksize * feed_nch * sizeof(int16_t));

    assert(feed_buff);

    while (task_flag) {
        size_t bytesIn = 0;
        esp_err_t result = i2s_channel_read(rx_handle, feed_buff, feed_chunksize * feed_nch * sizeof(int16_t), &bytesIn, portMAX_DELAY);

        if (result != ESP_OK || bytesIn <= 0) {
            ESP_LOGE(TAG, "I2S read error: %s", esp_err_to_name(result));
            continue;
        }

        if (bytesIn < feed_chunksize * feed_nch * sizeof(int16_t)) {
            ESP_LOGW(TAG, "I2S read less data than expected: %d bytes", bytesIn);
            memset(feed_buff + bytesIn / sizeof(int16_t), 0, (feed_chunksize * feed_nch - bytesIn / sizeof(int16_t)) * sizeof(int16_t));
        }

        // ====== 自动增益处理 & 打印最大幅度 ======
        int16_t max_val = 0;
        for (int i = 0; i < feed_chunksize * feed_nch; i++) {
            int32_t amplified = feed_buff[i] * 8; // 8倍放大
            if (amplified > 32767) amplified = 32767;
            if (amplified < -32768) amplified = -32768;
            feed_buff[i] = (int16_t)amplified;
            if (abs(feed_buff[i]) > max_val) max_val = abs(feed_buff[i]);
        }
        //ESP_LOGI(TAG, "Max amplitude: %d", max_val);

        // ====== feed 到 AFE 模块 ======
        afe_handle->feed(afe_data, feed_buff);
    }

    free(feed_buff);
    vTaskDelete(NULL);
}

void app_sr_detect_task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------detect start------------\n");

    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 

        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {//得到唤醒词
	        printf("model index:%d, word index:%d\n", res->wakenet_model_index, res->wake_word_index);
            printf("-----------LISTENING-----------\n");

            sr_result_t result = {
                .wakenet_mode = WAKENET_DETECTED,
                .state = ESP_MN_STATE_DETECTING,
                .command_id = 0,
            };
            // xQueueSend(g_result_que, &result, 10);
            // afe_handle->disable_wakenet(afe_data);
            xQueueSend(g_result_que, &result, 10);
            detect_flag = true;
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            detect_flag = true;
            afe_handle->disable_wakenet(afe_data);
        }
        if (true == detect_flag) {
            esp_mn_state_t mn_state = ESP_MN_STATE_DETECTING;

            mn_state = multinet->detect(model_data, res->data);

            if (ESP_MN_STATE_DETECTING == mn_state) {
                continue;
            }

            if (ESP_MN_STATE_TIMEOUT == mn_state) {//超时
                ESP_LOGW(TAG, "Time out");
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = 0,
                };
                xQueueSend(g_result_que, &result, 10);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = false;
                continue;
            }

            if (ESP_MN_STATE_DETECTED == mn_state) {//得到指令
                esp_mn_results_t *mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++) {
                    ESP_LOGI(TAG, "TOP %d, command_id: %d, phrase_id: %d, prob: %f",
                            i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->prob[i]);
                }

                int sr_command_id = mn_result->command_id[0];
                ESP_LOGI(TAG, "Deteted command : %d", sr_command_id);
                sr_result_t result = {
                    .wakenet_mode = WAKENET_NO_DETECT,
                    .state = mn_state,
                    .command_id = sr_command_id,
                };
                xQueueSend(g_result_que, &result, 10);
            }
        }
        
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

void sr_handler_task(void *pvParam)
{
    QueueHandle_t xQueue = (QueueHandle_t) pvParam;

    while (true) {
        sr_result_t result;
        xQueueReceive(xQueue, &result, portMAX_DELAY);

        ESP_LOGI(TAG, "cmd:%d, wakemode:%d,state:%d", result.command_id, result.wakenet_mode, result.state);

        if (ESP_MN_STATE_TIMEOUT == result.state) {
            ESP_LOGI(TAG, "timeout");
            ESP_ERROR_CHECK(audio_app_player_music_queue("/spiffs/goodbye.mp3"));
            continue;
        }

        if (WAKENET_DETECTED == result.wakenet_mode) {
            ESP_LOGI(TAG, "wakenet detected");
            ESP_ERROR_CHECK(audio_app_player_music_queue("/spiffs/master_user.mp3"));
            printf("%d",result.command_id);
            continue;
        }
        if(ESP_MN_STATE_DETECTED == result.state)
        {
            switch (result.command_id)
            {
                case 0:
                    /* 出粮 */
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    ESP_ERROR_CHECK(audio_app_player_music_queue("/spiffs/out_one_food.mp3"));
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(1);
                    break;
                case 1:
                    /* 请出粮 */
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    ESP_ERROR_CHECK(audio_app_player_music_queue("/spiffs/out_one_food.mp3"));
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(1);
                    break;
                case 2:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    /* 出一份粮 */
                    ESP_ERROR_CHECK(audio_app_player_music_queue("/spiffs/out_one_food.mp3"));
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(1);
                    break;
                case 3:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(2);
                    /* 出两份粮 */
                    break;
                case 4:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(3);   
                    /* 出三分粮 */
                    break;
                case 5:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(4);
                    /* 出四分粮 */
                    break;
                case 6:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    motor_control(MOTOR_FORWARD);
                    create_one_shot_timer(5);
                    /* 出五份粮 */
                    break;
                case 7:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    /* code */
                    ESP_ERROR_CHECK(audio_app_player_music_queue(MP3_SAVE_PATH));
                    break;
                case 8:
                    ESP_LOGI(TAG, "command_id: %d, phrase_id: %d", result.command_id, 0);
                    /* code */
                    break;
                default:
                    break;
            }
        }
        
    }
}

void app_sr_task_start(void)
{
    xTaskCreatePinnedToCore(&app_sr_feed_task, "feed_task", 8 * 1024, (void *)g_afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&app_sr_detect_task, "detect_task", 8 * 1024, (void *)g_afe_data, 4, NULL, 1);
    xTaskCreatePinnedToCore(&sr_handler_task, "SR Handler Task", 4 * 1024, g_result_que, 1, NULL, 1);
}
