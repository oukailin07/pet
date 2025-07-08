#include "http_tts.h"
#include "esp_log.h"

static const char *TAG = "TTS_EXAMPLE";

// ʾ�������ʹ���µ�TTS����
void tts_example_usage(void)
{
    // ��ȡIP��ַ������
    char ip_address[] = "192.168.1.100";
    http_get_ip_tts(ip_address);
    
    // ��ȡ�豸ID������
    char device_id[] = "ESP32_PET_001";
    http_get_device_id_tts(device_id);
    
    // ��ȡ�豸���������
    char password[] = "123456";
    http_get_password_tts(password);
    
    // ��ȡ�Զ����ı�������
    const char *custom_text = "��ӭʹ�����ܳ���ιʳ��";
    http_get_custom_tts(custom_text);
}

// ʾ����ʹ���Զ����ļ���
void tts_example_custom_filename(void)
{
    char ip_address[] = "192.168.1.100";
    char device_id[] = "ESP32_PET_001";
    char password[] = "123456";
    
    // ʹ���Զ����ļ���
    http_get_ip_tts_with_filename(ip_address, "/spiffs/my_ip.mp3");
    http_get_device_id_tts_with_filename(device_id, "/spiffs/my_device_id.mp3");
    http_get_password_tts_with_filename(password, "/spiffs/my_password.mp3");
    http_get_custom_tts_with_filename("�Զ�����Ϣ", "/spiffs/my_message.mp3");
}

// ʾ�������豸��ʼ��ʱ�����豸��Ϣ
void tts_play_device_info(const char *device_id, const char *password, const char *ip_address)
{
    ESP_LOGI(TAG, "�����豸��Ϣ");
    
    // �����豸ID
    http_get_device_id_tts((char*)device_id);
    
    // �ȴ�һ��ʱ��󲥷�����
    vTaskDelay(pdMS_TO_TICKS(2000));
    http_get_password_tts((char*)password);
    
    // �ȴ�һ��ʱ��󲥷�IP��ַ
    vTaskDelay(pdMS_TO_TICKS(2000));
    http_get_ip_tts((char*)ip_address);
}

// ʾ�����������ӳɹ���ʾ
void tts_play_connection_success(void)
{
    const char *success_msg = "�豸���ӳɹ������Կ�ʼʹ����";
    http_get_custom_tts(success_msg);
}

// ʾ�������Ŵ�����Ϣ
void tts_play_error_message(const char *error_msg)
{
    char full_msg[256];
    snprintf(full_msg, sizeof(full_msg), "��������%s", error_msg);
    http_get_custom_tts(full_msg);
}

// ʾ����Ϊ��ͬ����������ͬ����Ƶ�ļ�
void tts_create_scenario_files(void)
{
    // ������ӭ��Ƶ
    http_get_custom_tts_with_filename("��ӭʹ�����ܳ���ιʳ��", "/spiffs/welcome.mp3");
    
    // ����ιʳ��ʾ��Ƶ
    http_get_custom_tts_with_filename("����Ϊ����С����׼��ʳ��", "/spiffs/feeding.mp3");
    
    // ���������ʾ��Ƶ
    http_get_custom_tts_with_filename("ιʳ��ɣ�ף����С�����ò����", "/spiffs/complete.mp3");
    
    // ����������ʾ��Ƶ
    http_get_custom_tts_with_filename("����ʧ�ܣ������豸״̬", "/spiffs/error.mp3");
}

// ʾ������ȡ�ļ�·���ĸ�������
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