# HTTP TTS 功能说明

## 概述
本模块提供了基于百度语音合成API的TTS（Text-to-Speech）功能，可以将文本转换为语音播放。支持将不同的TTS请求保存到不同的MP3文件中，方便其他地方使用。

## 功能特性
- 支持IP地址语音播报
- 支持设备ID语音播报
- 支持设备密码语音播报
- 支持自定义文本语音播报
- 自动URL编码处理
- 内存管理优化
- **不同TTS请求保存到不同文件**
- **支持自定义文件名**

## 文件路径定义

### 默认文件路径
```c
#define MP3_SAVE_PATH_IP "/spiffs/baidu_tts_ip.mp3"           // IP地址音频文件
#define MP3_SAVE_PATH_DEVICE_ID "/spiffs/baidu_tts_device_id.mp3"  // 设备ID音频文件
#define MP3_SAVE_PATH_PASSWORD "/spiffs/baidu_tts_password.mp3"     // 设备密码音频文件
#define MP3_SAVE_PATH_CUSTOM "/spiffs/baidu_tts_custom.mp3"        // 自定义文本音频文件
#define MP3_SAVE_PATH_DEFAULT "/spiffs/baidu_tts.mp3"              // 默认音频文件
```

## API 函数

### 1. IP地址TTS
```c
void http_get_ip_tts(char *ip);
void http_get_ip_tts_with_filename(char *ip, const char *filename);
```
将IP地址转换为语音播报，格式为："设备 IP 是：xxx.xxx.xxx.xxx"

### 2. 设备ID TTS
```c
void http_get_device_id_tts(char *device_id);
void http_get_device_id_tts_with_filename(char *device_id, const char *filename);
```
将设备ID转换为语音播报，格式为："设备 ID 是：xxxxx"

### 3. 设备密码 TTS
```c
void http_get_password_tts(char *password);
void http_get_password_tts_with_filename(char *password, const char *filename);
```
将设备密码转换为语音播报，格式为："设备密码是：xxxxx"

### 4. 自定义文本 TTS
```c
void http_get_custom_tts(const char *text);
void http_get_custom_tts_with_filename(const char *text, const char *filename);
```
将任意文本转换为语音播报

## 使用示例

### 基本使用（使用默认文件名）
```c
#include "http_tts.h"

// 播报IP地址 - 保存到 /spiffs/baidu_tts_ip.mp3
char ip[] = "192.168.1.100";
http_get_ip_tts(ip);

// 播报设备ID - 保存到 /spiffs/baidu_tts_device_id.mp3
char device_id[] = "ESP32_PET_001";
http_get_device_id_tts(device_id);

// 播报设备密码 - 保存到 /spiffs/baidu_tts_password.mp3
char password[] = "123456";
http_get_password_tts(password);

// 播报自定义文本 - 保存到 /spiffs/baidu_tts_custom.mp3
http_get_custom_tts("欢迎使用智能宠物喂食器");
```

### 使用自定义文件名
```c
#include "http_tts.h"

// 使用自定义文件名
http_get_ip_tts_with_filename("192.168.1.100", "/spiffs/my_ip.mp3");
http_get_device_id_tts_with_filename("ESP32_PET_001", "/spiffs/my_device_id.mp3");
http_get_password_tts_with_filename("123456", "/spiffs/my_password.mp3");
http_get_custom_tts_with_filename("自定义消息", "/spiffs/my_message.mp3");
```

### 为不同场景创建音频文件
```c
#include "tts_example.h"

void create_scenario_audio_files(void)
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
```

### 获取文件路径
```c
#include "tts_example.h"

void get_file_paths(void)
{
    // 获取IP地址音频文件路径
    const char *ip_file = get_tts_file_path(TTS_FILE_IP);
    ESP_LOGI("TTS", "IP audio file: %s", ip_file);
    
    // 获取设备ID音频文件路径
    const char *device_id_file = get_tts_file_path(TTS_FILE_DEVICE_ID);
    ESP_LOGI("TTS", "Device ID audio file: %s", device_id_file);
    
    // 获取设备密码音频文件路径
    const char *password_file = get_tts_file_path(TTS_FILE_PASSWORD);
    ESP_LOGI("TTS", "Password audio file: %s", password_file);
}
```

### 设备初始化时播放完整信息
```c
#include "tts_example.h"

void device_init_tts(void)
{
    const char *device_id = "ESP32_PET_001";
    const char *password = "123456";
    const char *ip_address = "192.168.1.100";
    
    // 播放完整的设备信息
    tts_play_device_info(device_id, password, ip_address);
}
```

### 播放状态信息
```c
#include "tts_example.h"

void play_status_messages(void)
{
    // 播放连接成功提示
    tts_play_connection_success();
    
    // 播放错误信息
    tts_play_error_message("网络连接失败");
}
```

## 文件管理

### 生成的文件列表
使用默认函数时，会在SPIFFS中生成以下文件：
- `/spiffs/baidu_tts_ip.mp3` - IP地址音频
- `/spiffs/baidu_tts_device_id.mp3` - 设备ID音频
- `/spiffs/baidu_tts_password.mp3` - 设备密码音频
- `/spiffs/baidu_tts_custom.mp3` - 自定义文本音频

### 在其他地方使用音频文件
```c
#include "audio.h"

void play_audio_files(void)
{
    // 播放IP地址音频
    audio_app_player_music_queue("/spiffs/baidu_tts_ip.mp3");
    
    // 播放设备ID音频
    audio_app_player_music_queue("/spiffs/baidu_tts_device_id.mp3");
    
    // 播放设备密码音频
    audio_app_player_music_queue("/spiffs/baidu_tts_password.mp3");
    
    // 播放自定义音频
    audio_app_player_music_queue("/spiffs/baidu_tts_custom.mp3");
}
```

## 配置说明

### 百度API配置
在 `http_tts.c` 文件中配置以下参数：
- `url`: 百度TTS API地址
- `access_token`: 百度API访问令牌
- `formate`: API请求格式

### 音频参数
- 语言：中文 (zh)
- 语速：5 (中等速度)
- 音调：5 (中等音调)
- 音量：5 (中等音量)
- 发音人：4 (女声)
- 音频格式：MP3 (aue=3)

## 注意事项

1. **内存管理**: 函数内部会自动管理内存分配和释放，无需手动处理
2. **网络连接**: 需要确保设备已连接到互联网
3. **API限制**: 注意百度API的调用频率限制
4. **音频播放**: 生成的MP3文件会保存到指定的SPIFFS路径
5. **错误处理**: 函数内部包含错误处理，失败时会记录日志
6. **文件覆盖**: 每次调用都会覆盖同名文件
7. **存储空间**: 注意SPIFFS的存储空间限制

## 依赖项
- ESP-IDF HTTP客户端组件
- ESP-IDF SPIFFS文件系统
- 自定义URL编码模块 (`app_url_encode`)
- 音频播放模块 (`audio.h`)

## 文件结构
```
main/driver/http/
├── http_tts.c          # TTS功能实现
├── http_tts.h          # TTS功能头文件
├── tts_example.c       # 使用示例
├── tts_example.h       # 示例头文件
└── README_TTS.md       # 本文档
``` 