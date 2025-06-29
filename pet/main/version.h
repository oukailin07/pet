#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 版本号定义
#define FIRMWARE_VERSION_MAJOR    1
#define FIRMWARE_VERSION_MINOR    0
#define FIRMWARE_VERSION_PATCH    0
#define FIRMWARE_VERSION_BUILD    0
#define FIRMWARE_VERSION_SUFFIX   "stable"

// 协议版本定义
#define PROTOCOL_VERSION_MAJOR    1
#define PROTOCOL_VERSION_MINOR    0

// 硬件版本定义
#define HARDWARE_VERSION_MAJOR    1
#define HARDWARE_VERSION_MINOR    0

// 版本信息结构
typedef struct {
    uint8_t major;           // 主版本号
    uint8_t minor;           // 次版本号
    uint8_t patch;           // 补丁版本号
    uint16_t build;          // 构建号
    char suffix[16];         // 版本后缀
    char build_date[20];     // 构建日期
    char build_time[20];     // 构建时间
    char git_hash[16];       // Git提交哈希
} firmware_version_t;

// 协议版本信息结构
typedef struct {
    uint8_t major;           // 协议主版本号
    uint8_t minor;           // 协议次版本号
    char description[64];    // 协议描述
} protocol_version_t;

// 硬件版本信息结构
typedef struct {
    uint8_t major;           // 硬件主版本号
    uint8_t minor;           // 硬件次版本号
    char model[32];          // 硬件型号
    char description[64];    // 硬件描述
} hardware_version_t;

// 版本信息存储结构
typedef struct {
    firmware_version_t current_version;
    firmware_version_t backup_version;
    uint32_t install_time;
    uint32_t boot_count;
    bool is_stable;
} version_info_t;

// OTA升级状态枚举
typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_CHECKING,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_INSTALLING,
    OTA_STATUS_SUCCESS,
    OTA_STATUS_FAILED,
    OTA_STATUS_ROLLBACK
} ota_status_t;

// OTA升级信息结构
typedef struct {
    ota_status_t status;
    uint8_t progress;        // 进度百分比 0-100
    uint32_t error_code;     // 错误代码
    char error_message[128]; // 错误描述
    char target_version[32]; // 目标版本
    char download_url[256];  // 下载URL
    uint32_t file_size;      // 文件大小
    char checksum[64];       // 校验和
} ota_info_t;

// 函数声明

/**
 * @brief 获取当前固件版本信息
 * @return 固件版本信息指针
 */
const firmware_version_t* get_firmware_version(void);

/**
 * @brief 获取当前协议版本信息
 * @return 协议版本信息指针
 */
const protocol_version_t* get_protocol_version(void);

/**
 * @brief 获取当前硬件版本信息
 * @return 硬件版本信息指针
 */
const hardware_version_t* get_hardware_version(void);

/**
 * @brief 获取版本信息字符串
 * @param version 版本信息结构
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 版本字符串长度
 */
int get_version_string(const firmware_version_t *version, char *buffer, size_t size);

/**
 * @brief 检查固件兼容性
 * @param current 当前版本
 * @param target 目标版本
 * @return true表示兼容，false表示不兼容
 */
bool check_firmware_compatibility(const firmware_version_t *current, 
                                 const firmware_version_t *target);

/**
 * @brief 检查协议兼容性
 * @param client 客户端版本
 * @param server 服务器版本
 * @return true表示兼容，false表示不兼容
 */
bool check_protocol_compatibility(const protocol_version_t *client, 
                                 const protocol_version_t *server);

/**
 * @brief 初始化版本管理模块
 * @return ESP_OK成功，其他值失败
 */
esp_err_t version_manager_init(void);

/**
 * @brief 保存版本信息到NVS
 * @param version_info 版本信息
 * @return ESP_OK成功，其他值失败
 */
esp_err_t save_version_info(const version_info_t *version_info);

/**
 * @brief 从NVS加载版本信息
 * @param version_info 版本信息输出
 * @return ESP_OK成功，其他值失败
 */
esp_err_t load_version_info(version_info_t *version_info);

/**
 * @brief 更新版本信息
 * @param new_version 新版本信息
 * @return ESP_OK成功，其他值失败
 */
esp_err_t update_version_info(const firmware_version_t *new_version);

/**
 * @brief 获取OTA升级信息
 * @param ota_info OTA信息输出
 * @return ESP_OK成功，其他值失败
 */
esp_err_t get_ota_info(ota_info_t *ota_info);

/**
 * @brief 更新OTA状态
 * @param status OTA状态
 * @param progress 进度百分比
 * @param error_code 错误代码
 * @param error_message 错误描述
 * @return ESP_OK成功，其他值失败
 */
esp_err_t update_ota_status(ota_status_t status, uint8_t progress, 
                           uint32_t error_code, const char *error_message);

/**
 * @brief 检查是否有可用更新
 * @param current_version 当前版本
 * @param latest_version 最新版本输出
 * @param download_url 下载URL输出
 * @return true表示有更新，false表示无更新
 */
bool check_for_updates(const char *current_version, char *latest_version, 
                      char *download_url);

/**
 * @brief 执行版本回滚
 * @param target_version 目标版本
 * @return ESP_OK成功，其他值失败
 */
esp_err_t rollback_version(const char *target_version);

/**
 * @brief 获取版本历史记录
 * @param versions 版本数组
 * @param max_count 最大版本数量
 * @return 实际版本数量
 */
int get_version_history(firmware_version_t *versions, int max_count);

/**
 * @brief 清理版本历史记录
 * @param keep_count 保留的版本数量
 * @return ESP_OK成功，其他值失败
 */
esp_err_t cleanup_version_history(int keep_count);

#ifdef __cplusplus
}
#endif 