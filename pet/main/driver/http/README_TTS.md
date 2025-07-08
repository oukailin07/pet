# HTTP TTS ����˵��

## ����
��ģ���ṩ�˻��ڰٶ������ϳ�API��TTS��Text-to-Speech�����ܣ����Խ��ı�ת��Ϊ�������š�֧�ֽ���ͬ��TTS���󱣴浽��ͬ��MP3�ļ��У����������ط�ʹ�á�

## ��������
- ֧��IP��ַ��������
- ֧���豸ID��������
- ֧���豸������������
- ֧���Զ����ı���������
- �Զ�URL���봦��
- �ڴ�����Ż�
- **��ͬTTS���󱣴浽��ͬ�ļ�**
- **֧���Զ����ļ���**

## �ļ�·������

### Ĭ���ļ�·��
```c
#define MP3_SAVE_PATH_IP "/spiffs/baidu_tts_ip.mp3"           // IP��ַ��Ƶ�ļ�
#define MP3_SAVE_PATH_DEVICE_ID "/spiffs/baidu_tts_device_id.mp3"  // �豸ID��Ƶ�ļ�
#define MP3_SAVE_PATH_PASSWORD "/spiffs/baidu_tts_password.mp3"     // �豸������Ƶ�ļ�
#define MP3_SAVE_PATH_CUSTOM "/spiffs/baidu_tts_custom.mp3"        // �Զ����ı���Ƶ�ļ�
#define MP3_SAVE_PATH_DEFAULT "/spiffs/baidu_tts.mp3"              // Ĭ����Ƶ�ļ�
```

## API ����

### 1. IP��ַTTS
```c
void http_get_ip_tts(char *ip);
void http_get_ip_tts_with_filename(char *ip, const char *filename);
```
��IP��ַת��Ϊ������������ʽΪ��"�豸 IP �ǣ�xxx.xxx.xxx.xxx"

### 2. �豸ID TTS
```c
void http_get_device_id_tts(char *device_id);
void http_get_device_id_tts_with_filename(char *device_id, const char *filename);
```
���豸IDת��Ϊ������������ʽΪ��"�豸 ID �ǣ�xxxxx"

### 3. �豸���� TTS
```c
void http_get_password_tts(char *password);
void http_get_password_tts_with_filename(char *password, const char *filename);
```
���豸����ת��Ϊ������������ʽΪ��"�豸�����ǣ�xxxxx"

### 4. �Զ����ı� TTS
```c
void http_get_custom_tts(const char *text);
void http_get_custom_tts_with_filename(const char *text, const char *filename);
```
�������ı�ת��Ϊ��������

## ʹ��ʾ��

### ����ʹ�ã�ʹ��Ĭ���ļ�����
```c
#include "http_tts.h"

// ����IP��ַ - ���浽 /spiffs/baidu_tts_ip.mp3
char ip[] = "192.168.1.100";
http_get_ip_tts(ip);

// �����豸ID - ���浽 /spiffs/baidu_tts_device_id.mp3
char device_id[] = "ESP32_PET_001";
http_get_device_id_tts(device_id);

// �����豸���� - ���浽 /spiffs/baidu_tts_password.mp3
char password[] = "123456";
http_get_password_tts(password);

// �����Զ����ı� - ���浽 /spiffs/baidu_tts_custom.mp3
http_get_custom_tts("��ӭʹ�����ܳ���ιʳ��");
```

### ʹ���Զ����ļ���
```c
#include "http_tts.h"

// ʹ���Զ����ļ���
http_get_ip_tts_with_filename("192.168.1.100", "/spiffs/my_ip.mp3");
http_get_device_id_tts_with_filename("ESP32_PET_001", "/spiffs/my_device_id.mp3");
http_get_password_tts_with_filename("123456", "/spiffs/my_password.mp3");
http_get_custom_tts_with_filename("�Զ�����Ϣ", "/spiffs/my_message.mp3");
```

### Ϊ��ͬ����������Ƶ�ļ�
```c
#include "tts_example.h"

void create_scenario_audio_files(void)
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
```

### ��ȡ�ļ�·��
```c
#include "tts_example.h"

void get_file_paths(void)
{
    // ��ȡIP��ַ��Ƶ�ļ�·��
    const char *ip_file = get_tts_file_path(TTS_FILE_IP);
    ESP_LOGI("TTS", "IP audio file: %s", ip_file);
    
    // ��ȡ�豸ID��Ƶ�ļ�·��
    const char *device_id_file = get_tts_file_path(TTS_FILE_DEVICE_ID);
    ESP_LOGI("TTS", "Device ID audio file: %s", device_id_file);
    
    // ��ȡ�豸������Ƶ�ļ�·��
    const char *password_file = get_tts_file_path(TTS_FILE_PASSWORD);
    ESP_LOGI("TTS", "Password audio file: %s", password_file);
}
```

### �豸��ʼ��ʱ����������Ϣ
```c
#include "tts_example.h"

void device_init_tts(void)
{
    const char *device_id = "ESP32_PET_001";
    const char *password = "123456";
    const char *ip_address = "192.168.1.100";
    
    // �����������豸��Ϣ
    tts_play_device_info(device_id, password, ip_address);
}
```

### ����״̬��Ϣ
```c
#include "tts_example.h"

void play_status_messages(void)
{
    // �������ӳɹ���ʾ
    tts_play_connection_success();
    
    // ���Ŵ�����Ϣ
    tts_play_error_message("��������ʧ��");
}
```

## �ļ�����

### ���ɵ��ļ��б�
ʹ��Ĭ�Ϻ���ʱ������SPIFFS�����������ļ���
- `/spiffs/baidu_tts_ip.mp3` - IP��ַ��Ƶ
- `/spiffs/baidu_tts_device_id.mp3` - �豸ID��Ƶ
- `/spiffs/baidu_tts_password.mp3` - �豸������Ƶ
- `/spiffs/baidu_tts_custom.mp3` - �Զ����ı���Ƶ

### �������ط�ʹ����Ƶ�ļ�
```c
#include "audio.h"

void play_audio_files(void)
{
    // ����IP��ַ��Ƶ
    audio_app_player_music_queue("/spiffs/baidu_tts_ip.mp3");
    
    // �����豸ID��Ƶ
    audio_app_player_music_queue("/spiffs/baidu_tts_device_id.mp3");
    
    // �����豸������Ƶ
    audio_app_player_music_queue("/spiffs/baidu_tts_password.mp3");
    
    // �����Զ�����Ƶ
    audio_app_player_music_queue("/spiffs/baidu_tts_custom.mp3");
}
```

## ����˵��

### �ٶ�API����
�� `http_tts.c` �ļ����������²�����
- `url`: �ٶ�TTS API��ַ
- `access_token`: �ٶ�API��������
- `formate`: API�����ʽ

### ��Ƶ����
- ���ԣ����� (zh)
- ���٣�5 (�е��ٶ�)
- ������5 (�е�����)
- ������5 (�е�����)
- �����ˣ�4 (Ů��)
- ��Ƶ��ʽ��MP3 (aue=3)

## ע������

1. **�ڴ����**: �����ڲ����Զ������ڴ������ͷţ������ֶ�����
2. **��������**: ��Ҫȷ���豸�����ӵ�������
3. **API����**: ע��ٶ�API�ĵ���Ƶ������
4. **��Ƶ����**: ���ɵ�MP3�ļ��ᱣ�浽ָ����SPIFFS·��
5. **������**: �����ڲ�����������ʧ��ʱ���¼��־
6. **�ļ�����**: ÿ�ε��ö��Ḳ��ͬ���ļ�
7. **�洢�ռ�**: ע��SPIFFS�Ĵ洢�ռ�����

## ������
- ESP-IDF HTTP�ͻ������
- ESP-IDF SPIFFS�ļ�ϵͳ
- �Զ���URL����ģ�� (`app_url_encode`)
- ��Ƶ����ģ�� (`audio.h`)

## �ļ��ṹ
```
main/driver/http/
������ http_tts.c          # TTS����ʵ��
������ http_tts.h          # TTS����ͷ�ļ�
������ tts_example.c       # ʹ��ʾ��
������ tts_example.h       # ʾ��ͷ�ļ�
������ README_TTS.md       # ���ĵ�
``` 