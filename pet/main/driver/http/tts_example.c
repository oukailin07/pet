#include "http_tts.h"
#include "esp_log.h"

static const char *TAG = "TTS_EXAMPLE";

// 示例：如何使用新的TTS函数
void tts_example_usage(void)
{
    // 获取IP地址的语音
    char ip_address[] = "192.168.1.100";
    http_get_ip_tts(ip_address);
    
    // 获取设备ID的语音
    char device_id[] = "ESP32_PET_001";
    http_get_device_id_tts(device_id);
    
    // 获取设备密码的语音
    char password[] = "123456";
    http_get_password_tts(password);
    
    // 获取自定义文本的语音
    const char *custom_text = "欢迎使用智能宠物喂食器";
    http_get_custom_tts(custom_text);
}

// 示例：使用自定义文件名
void tts_example_custom_filename(void)
{
    char ip_address[] = "192.168.1.100";
    char device_id[] = "ESP32_PET_001";
    char password[] = "123456";
    
    // 使用自定义文件名
    http_get_ip_tts_with_filename(ip_address, "/spiffs/my_ip.mp3");
    http_get_device_id_tts_with_filename(device_id, "/spiffs/my_device_id.mp3");
    http_get_password_tts_with_filename(password, "/spiffs/my_password.mp3");
    http_get_custom_tts_with_filename("自定义消息", "/spiffs/my_message.mp3");
}

// 示例：在设备初始化时播放设备信息
void tts_play_device_info(const char *device_id, const char *password, const char *ip_address)
{
    ESP_LOGI(TAG, "播放设备信息");
    
    // 播放设备ID
    http_get_device_id_tts((char*)device_id);
    
    // 等待一段时间后播放密码
    vTaskDelay(pdMS_TO_TICKS(2000));
    http_get_password_tts((char*)password);
    
    // 等待一段时间后播放IP地址
    vTaskDelay(pdMS_TO_TICKS(2000));
    http_get_ip_tts((char*)ip_address);
}

// 示例：播放连接成功提示
void tts_play_connection_success(void)
{
    const char *success_msg = "设备连接成功，可以开始使用了";
    http_get_custom_tts(success_msg);
}

// 示例：播放错误信息
void tts_play_error_message(const char *error_msg)
{
    char full_msg[256];
    snprintf(full_msg, sizeof(full_msg), "发生错误：%s", error_msg);
    http_get_custom_tts(full_msg);
}

// 示例：为不同场景创建不同的音频文件
void tts_create_scenario_files(void)
{
    // 创建欢迎音频
    http_get_custom_tts_with_filename("欢迎使用智能宠物喂食器", "/spiffs/welcome.mp3");
    
    // 创建喂食提示音频
    http_get_custom_tts_with_filename("正在为您的小宠物准备食物", "/spiffs/feeding.mp3");
    
    // 创建完成提示音频
    http_get_custom_tts_with_filename("喂食完成，祝您的小宠物用餐愉快", "/spiffs/complete.mp3");
    
    // 创建错误提示音频
    http_get_custom_tts_with_filename("操作失败，请检查设备状态", "/spiffs/error.mp3");
}

// 示例：获取文件路径的辅助函数
const char* get_tts_file_path(tts_file_type_t type)
{
    switch(type) {
        case TTS_FILE_IP:
            return MP3_SAVE_PATH_IP;
        case TTS_FILE_DEVICE_ID:
            return MP3_SAVE_PATH_DEVICE_ID;
        case TTS_FILE_PASSWORD:
            return MP3_SAVE_PATH_PASSWORD;
        case TTS_FILE_CUSTOM:
            return MP3_SAVE_PATH_CUSTOM;
        default:
            return MP3_SAVE_PATH_DEFAULT;
    }
} 