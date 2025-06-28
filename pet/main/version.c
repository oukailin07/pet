#include "version.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <time.h>
#include <inttypes.h>

static const char *TAG = "VERSION";

// 全局版本信息
static firmware_version_t g_firmware_version = {
    .major = FIRMWARE_VERSION_MAJOR,
    .minor = FIRMWARE_VERSION_MINOR,
    .patch = FIRMWARE_VERSION_PATCH,
    .build = FIRMWARE_VERSION_BUILD,
    .suffix = FIRMWARE_VERSION_SUFFIX,
    .build_date = __DATE__,
    .build_time = __TIME__,
    .git_hash = "unknown"
};

static protocol_version_t g_protocol_version = {
    .major = PROTOCOL_VERSION_MAJOR,
    .minor = PROTOCOL_VERSION_MINOR,
    .description = "Pet Feeder Protocol v1.0"
};

static hardware_version_t g_hardware_version = {
    .major = HARDWARE_VERSION_MAJOR,
    .minor = HARDWARE_VERSION_MINOR,
    .model = "ESP32-PetFeeder",
    .description = "ESP32-based Pet Feeder Device"
};

// NVS命名空间
#define VERSION_NAMESPACE "version"
#define VERSION_INFO_KEY "version_info"
#define OTA_INFO_KEY "ota_info"

const firmware_version_t* get_firmware_version(void) {
    return &g_firmware_version;
}

const protocol_version_t* get_protocol_version(void) {
    return &g_protocol_version;
}

const hardware_version_t* get_hardware_version(void) {
    return &g_hardware_version;
}

int get_version_string(const firmware_version_t *version, char *buffer, size_t size) {
    if (!version || !buffer || size == 0) {
        return -1;
    }
    
    int len = snprintf(buffer, size, "v%d.%d.%d", 
                      version->major, version->minor, version->patch);
    
    if (version->build > 0 && len < size - 1) {
        len += snprintf(buffer + len, size - len, "-%d", version->build);
    }
    
    if (strlen(version->suffix) > 0 && len < size - 1) {
        len += snprintf(buffer + len, size - len, "-%s", version->suffix);
    }
    
    return len;
}

bool check_firmware_compatibility(const firmware_version_t *current, 
                                 const firmware_version_t *target) {
    if (!current || !target) {
        return false;
    }
    
    // 主版本号必须相同
    if (current->major != target->major) {
        ESP_LOGW(TAG, "Major version mismatch: current=%d, target=%d", 
                current->major, target->major);
        return false;
    }
    
    // 目标版本不能低于当前版本
    if (target->minor < current->minor) {
        ESP_LOGW(TAG, "Target minor version too low: current=%d, target=%d", 
                current->minor, target->minor);
        return false;
    }
    
    if (target->minor == current->minor && target->patch < current->patch) {
        ESP_LOGW(TAG, "Target patch version too low: current=%d, target=%d", 
                current->patch, target->patch);
        return false;
    }
    
    return true;
}

bool check_protocol_compatibility(const protocol_version_t *client, 
                                 const protocol_version_t *server) {
    if (!client || !server) {
        return false;
    }
    
    // 主版本号必须相同
    if (client->major != server->major) {
        ESP_LOGW(TAG, "Protocol major version mismatch: client=%d, server=%d", 
                client->major, server->major);
        return false;
    }
    
    // 客户端版本不能低于服务器版本
    if (client->minor < server->minor) {
        ESP_LOGW(TAG, "Client protocol version too low: client=%d, server=%d", 
                client->minor, server->minor);
        return false;
    }
    
    return true;
}

esp_err_t version_manager_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 加载版本信息
    version_info_t version_info;
    if (load_version_info(&version_info) == ESP_OK) {
        // 检查是否需要更新启动计数
        version_info.boot_count++;
        save_version_info(&version_info);
        ESP_LOGI(TAG, "Boot count: %" PRIu32, version_info.boot_count);
    } else {
        // 初始化版本信息
        memset(&version_info, 0, sizeof(version_info));
        version_info.current_version = g_firmware_version;
        version_info.install_time = time(NULL);
        version_info.boot_count = 1;
        version_info.is_stable = true;
        save_version_info(&version_info);
        ESP_LOGI(TAG, "Initialized version info");
    }
    
    // 打印版本信息
    char version_str[64];
    get_version_string(&g_firmware_version, version_str, sizeof(version_str));
    ESP_LOGI(TAG, "Firmware version: %s", version_str);
    ESP_LOGI(TAG, "Protocol version: %d.%d", g_protocol_version.major, g_protocol_version.minor);
    ESP_LOGI(TAG, "Hardware version: %d.%d", g_hardware_version.major, g_hardware_version.minor);
    
    return ESP_OK;
}

esp_err_t save_version_info(const version_info_t *version_info) {
    if (!version_info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(VERSION_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = nvs_set_blob(nvs_handle, VERSION_INFO_KEY, version_info, sizeof(version_info_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save version info: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Version info saved successfully");
    }
    
    return ret;
}

esp_err_t load_version_info(version_info_t *version_info) {
    if (!version_info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(VERSION_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    size_t required_size = sizeof(version_info_t);
    ret = nvs_get_blob(nvs_handle, VERSION_INFO_KEY, version_info, &required_size);
    nvs_close(nvs_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load version info: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Version info loaded successfully");
    return ESP_OK;
}

esp_err_t update_version_info(const firmware_version_t *new_version) {
    if (!new_version) {
        return ESP_ERR_INVALID_ARG;
    }
    
    version_info_t version_info;
    esp_err_t ret = load_version_info(&version_info);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 备份当前版本
    version_info.backup_version = version_info.current_version;
    
    // 更新当前版本
    version_info.current_version = *new_version;
    version_info.install_time = time(NULL);
    version_info.is_stable = true;
    
    return save_version_info(&version_info);
}

esp_err_t get_ota_info(ota_info_t *ota_info) {
    if (!ota_info) {
        return ESP_ERR_INVALID_ARG;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(VERSION_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    size_t required_size = sizeof(ota_info_t);
    ret = nvs_get_blob(nvs_handle, OTA_INFO_KEY, ota_info, &required_size);
    nvs_close(nvs_handle);
    
    if (ret != ESP_OK) {
        // 初始化默认值
        memset(ota_info, 0, sizeof(ota_info_t));
        ota_info->status = OTA_STATUS_IDLE;
    }
    
    return ESP_OK;
}

esp_err_t update_ota_status(ota_status_t status, uint8_t progress, 
                           uint32_t error_code, const char *error_message) {
    ota_info_t ota_info;
    esp_err_t ret = get_ota_info(&ota_info);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ota_info.status = status;
    ota_info.progress = progress;
    ota_info.error_code = error_code;
    
    if (error_message) {
        strncpy(ota_info.error_message, error_message, sizeof(ota_info.error_message) - 1);
        ota_info.error_message[sizeof(ota_info.error_message) - 1] = '\0';
    }
    
    nvs_handle_t nvs_handle;
    ret = nvs_open(VERSION_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = nvs_set_blob(nvs_handle, OTA_INFO_KEY, &ota_info, sizeof(ota_info_t));
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA status updated: %d, progress: %d%%, error: %" PRIu32, 
                status, progress, error_code);
    }
    
    return ret;
}

bool check_for_updates(const char *current_version, char *latest_version, 
                      char *download_url) {
    // 这里应该实现与服务器的通信来检查更新
    // 目前返回false表示无更新
    ESP_LOGI(TAG, "Checking for updates (current: %s)", current_version ? current_version : "unknown");
    
    if (latest_version) {
        strcpy(latest_version, "");
    }
    if (download_url) {
        strcpy(download_url, "");
    }
    
    return false;
}

esp_err_t rollback_version(const char *target_version) {
    if (!target_version) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Rolling back to version: %s", target_version);
    
    // 这里应该实现版本回滚逻辑
    // 1. 验证目标版本是否存在
    // 2. 切换到备份分区
    // 3. 重启设备
    
    return ESP_OK;
}

int get_version_history(firmware_version_t *versions, int max_count) {
    if (!versions || max_count <= 0) {
        return 0;
    }
    
    // 这里应该从NVS中读取版本历史记录
    // 目前只返回当前版本
    if (max_count >= 1) {
        versions[0] = g_firmware_version;
        return 1;
    }
    
    return 0;
}

esp_err_t cleanup_version_history(int keep_count) {
    if (keep_count < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Cleaning up version history, keeping %d versions", keep_count);
    
    // 这里应该实现版本历史清理逻辑
    // 删除旧的版本记录，只保留最近的几个版本
    
    return ESP_OK;
} 