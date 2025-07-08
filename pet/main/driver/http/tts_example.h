#ifndef TTS_EXAMPLE_H
#define TTS_EXAMPLE_H

#include "esp_err.h"

// �ļ�����ö��
typedef enum {
    TTS_FILE_IP,
    TTS_FILE_DEVICE_ID,
    TTS_FILE_PASSWORD,
    TTS_FILE_CUSTOM,
    TTS_FILE_DEFAULT
} tts_file_type_t;

// ʾ�������ʹ���µ�TTS����
void tts_example_usage(void);

// ʾ����ʹ���Զ����ļ���
void tts_example_custom_filename(void);

// ʾ�������豸��ʼ��ʱ�����豸��Ϣ
void tts_play_device_info(const char *device_id, const char *password, const char *ip_address);

// ʾ�����������ӳɹ���ʾ
void tts_play_connection_success(void);

// ʾ�������Ŵ�����Ϣ
void tts_play_error_message(const char *error_msg);

// ʾ����Ϊ��ͬ����������ͬ����Ƶ�ļ�
void tts_create_scenario_files(void);

// ʾ������ȡ�ļ�·���ĸ�������
const char* get_tts_file_path(tts_file_type_t type);

#endif // TTS_EXAMPLE_H 