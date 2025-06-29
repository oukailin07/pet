#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// �汾�Ŷ���
#define FIRMWARE_VERSION_MAJOR    1
#define FIRMWARE_VERSION_MINOR    0
#define FIRMWARE_VERSION_PATCH    0
#define FIRMWARE_VERSION_BUILD    0
#define FIRMWARE_VERSION_SUFFIX   "stable"

// Э��汾����
#define PROTOCOL_VERSION_MAJOR    1
#define PROTOCOL_VERSION_MINOR    0

// Ӳ���汾����
#define HARDWARE_VERSION_MAJOR    1
#define HARDWARE_VERSION_MINOR    0

// �汾��Ϣ�ṹ
typedef struct {
    uint8_t major;           // ���汾��
    uint8_t minor;           // �ΰ汾��
    uint8_t patch;           // �����汾��
    uint16_t build;          // ������
    char suffix[16];         // �汾��׺
    char build_date[20];     // ��������
    char build_time[20];     // ����ʱ��
    char git_hash[16];       // Git�ύ��ϣ
} firmware_version_t;

// Э��汾��Ϣ�ṹ
typedef struct {
    uint8_t major;           // Э�����汾��
    uint8_t minor;           // Э��ΰ汾��
    char description[64];    // Э������
} protocol_version_t;

// Ӳ���汾��Ϣ�ṹ
typedef struct {
    uint8_t major;           // Ӳ�����汾��
    uint8_t minor;           // Ӳ���ΰ汾��
    char model[32];          // Ӳ���ͺ�
    char description[64];    // Ӳ������
} hardware_version_t;

// �汾��Ϣ�洢�ṹ
typedef struct {
    firmware_version_t current_version;
    firmware_version_t backup_version;
    uint32_t install_time;
    uint32_t boot_count;
    bool is_stable;
} version_info_t;

// OTA����״̬ö��
typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_CHECKING,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_INSTALLING,
    OTA_STATUS_SUCCESS,
    OTA_STATUS_FAILED,
    OTA_STATUS_ROLLBACK
} ota_status_t;

// OTA������Ϣ�ṹ
typedef struct {
    ota_status_t status;
    uint8_t progress;        // ���Ȱٷֱ� 0-100
    uint32_t error_code;     // �������
    char error_message[128]; // ��������
    char target_version[32]; // Ŀ��汾
    char download_url[256];  // ����URL
    uint32_t file_size;      // �ļ���С
    char checksum[64];       // У���
} ota_info_t;

// ��������

/**
 * @brief ��ȡ��ǰ�̼��汾��Ϣ
 * @return �̼��汾��Ϣָ��
 */
const firmware_version_t* get_firmware_version(void);

/**
 * @brief ��ȡ��ǰЭ��汾��Ϣ
 * @return Э��汾��Ϣָ��
 */
const protocol_version_t* get_protocol_version(void);

/**
 * @brief ��ȡ��ǰӲ���汾��Ϣ
 * @return Ӳ���汾��Ϣָ��
 */
const hardware_version_t* get_hardware_version(void);

/**
 * @brief ��ȡ�汾��Ϣ�ַ���
 * @param version �汾��Ϣ�ṹ
 * @param buffer ���������
 * @param size ��������С
 * @return �汾�ַ�������
 */
int get_version_string(const firmware_version_t *version, char *buffer, size_t size);

/**
 * @brief ���̼�������
 * @param current ��ǰ�汾
 * @param target Ŀ��汾
 * @return true��ʾ���ݣ�false��ʾ������
 */
bool check_firmware_compatibility(const firmware_version_t *current, 
                                 const firmware_version_t *target);

/**
 * @brief ���Э�������
 * @param client �ͻ��˰汾
 * @param server �������汾
 * @return true��ʾ���ݣ�false��ʾ������
 */
bool check_protocol_compatibility(const protocol_version_t *client, 
                                 const protocol_version_t *server);

/**
 * @brief ��ʼ���汾����ģ��
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t version_manager_init(void);

/**
 * @brief ����汾��Ϣ��NVS
 * @param version_info �汾��Ϣ
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t save_version_info(const version_info_t *version_info);

/**
 * @brief ��NVS���ذ汾��Ϣ
 * @param version_info �汾��Ϣ���
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t load_version_info(version_info_t *version_info);

/**
 * @brief ���°汾��Ϣ
 * @param new_version �°汾��Ϣ
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t update_version_info(const firmware_version_t *new_version);

/**
 * @brief ��ȡOTA������Ϣ
 * @param ota_info OTA��Ϣ���
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t get_ota_info(ota_info_t *ota_info);

/**
 * @brief ����OTA״̬
 * @param status OTA״̬
 * @param progress ���Ȱٷֱ�
 * @param error_code �������
 * @param error_message ��������
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t update_ota_status(ota_status_t status, uint8_t progress, 
                           uint32_t error_code, const char *error_message);

/**
 * @brief ����Ƿ��п��ø���
 * @param current_version ��ǰ�汾
 * @param latest_version ���°汾���
 * @param download_url ����URL���
 * @return true��ʾ�и��£�false��ʾ�޸���
 */
bool check_for_updates(const char *current_version, char *latest_version, 
                      char *download_url);

/**
 * @brief ִ�а汾�ع�
 * @param target_version Ŀ��汾
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t rollback_version(const char *target_version);

/**
 * @brief ��ȡ�汾��ʷ��¼
 * @param versions �汾����
 * @param max_count ���汾����
 * @return ʵ�ʰ汾����
 */
int get_version_history(firmware_version_t *versions, int max_count);

/**
 * @brief ����汾��ʷ��¼
 * @param keep_count �����İ汾����
 * @return ESP_OK�ɹ�������ֵʧ��
 */
esp_err_t cleanup_version_history(int keep_count);

#ifdef __cplusplus
}
#endif 