#ifndef TTS_EXAMPLE_H
#define TTS_EXAMPLE_H

#include "esp_err.h"

// 文件类型枚举
typedef enum {
    TTS_FILE_IP,
    TTS_FILE_DEVICE_ID,
    TTS_FILE_PASSWORD,
    TTS_FILE_CUSTOM,
    TTS_FILE_DEFAULT
} tts_file_type_t;

// 示例：如何使用新的TTS函数
void tts_example_usage(void);

// 示例：使用自定义文件名
void tts_example_custom_filename(void);

// 示例：在设备初始化时播放设备信息
void tts_play_device_info(const char *device_id, const char *password, const char *ip_address);

// 示例：播放连接成功提示
void tts_play_connection_success(void);

// 示例：播放错误信息
void tts_play_error_message(const char *error_msg);

// 示例：为不同场景创建不同的音频文件
void tts_create_scenario_files(void);

// 示例：获取文件路径的辅助函数
const char* get_tts_file_path(tts_file_type_t type);

#endif // TTS_EXAMPLE_H 