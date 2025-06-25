 #include "time_utils.h"
#include "esp_log.h"
#include <time.h>
#include <string.h>

static const char *TAG = "TIME_UTILS";

// 星期名称数组
static const char* weekdays[] = {
    "星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"
};

// 英文星期名称数组
static const char* weekdays_en[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};

/**
 * @brief 获取当前时间的时分秒和星期
 * @param time_info 时间信息结构体指针
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t get_current_time_info(time_info_t *time_info)
{
    if (time_info == NULL) {
        ESP_LOGE(TAG, "time_info pointer is NULL");
        return ESP_FAIL;
    }

    time_t now;
    struct tm timeinfo;
    
    // 获取当前时间
    time(&now);
    
    // 转换为本地时间
    if (localtime_r(&now, &timeinfo) == NULL) {
        ESP_LOGE(TAG, "Failed to get local time");
        return ESP_FAIL;
    }

    // 填充时间信息
    time_info->hour = timeinfo.tm_hour;
    time_info->minute = timeinfo.tm_min;
    time_info->second = timeinfo.tm_sec;
    time_info->weekday = timeinfo.tm_wday;  // 0=星期日, 1=星期一, ..., 6=星期六
    time_info->year = timeinfo.tm_year + 1900;  // tm_year是从1900年开始的
    time_info->month = timeinfo.tm_mon + 1;     // tm_mon是从0开始的
    time_info->day = timeinfo.tm_mday;

    return ESP_OK;
}

/**
 * @brief 获取格式化的时间字符串
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param format 格式类型 (0: 中文格式, 1: 英文格式, 2: 数字格式)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t get_formatted_time_string(char *buffer, size_t buffer_size, int format)
{
    if (buffer == NULL || buffer_size == 0) {
        ESP_LOGE(TAG, "Invalid buffer parameters");
        return ESP_FAIL;
    }

    time_info_t time_info;
    esp_err_t ret = get_current_time_info(&time_info);
    if (ret != ESP_OK) {
        return ret;
    }

    switch (format) {
        case 0: // 中文格式: 14:30:25 星期一
            snprintf(buffer, buffer_size, "%02d:%02d:%02d %s", 
                     time_info.hour, time_info.minute, time_info.second, 
                     weekdays[time_info.weekday]);
            break;
            
        case 1: // 英文格式: 14:30:25 Monday
            snprintf(buffer, buffer_size, "%02d:%02d:%02d %s", 
                     time_info.hour, time_info.minute, time_info.second, 
                     weekdays_en[time_info.weekday]);
            break;
            
        case 2: // 数字格式: 14:30:25 1
            snprintf(buffer, buffer_size, "%02d:%02d:%02d %d", 
                     time_info.hour, time_info.minute, time_info.second, 
                     time_info.weekday);
            break;
            
        case 3: // 完整中文格式: 2024年1月15日 14:30:25 星期一
            snprintf(buffer, buffer_size, "%d年%d月%d日 %02d:%02d:%02d %s", 
                     time_info.year, time_info.month, time_info.day,
                     time_info.hour, time_info.minute, time_info.second, 
                     weekdays[time_info.weekday]);
            break;
            
        default:
            ESP_LOGE(TAG, "Invalid format type: %d", format);
            return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief 获取当前时间戳
 * @return time_t 当前时间戳
 */
time_t get_current_timestamp(void)
{
    time_t now;
    time(&now);
    return now;
}

/**
 * @brief 检查是否为工作时间 (9:00-18:00)
 * @return bool true为工作时间，false为非工作时间
 */
bool is_work_time(void)
{
    time_info_t time_info;
    if (get_current_time_info(&time_info) != ESP_OK) {
        return false;
    }
    
    // 检查是否为工作日 (周一到周五)
    if (time_info.weekday == 0 || time_info.weekday == 6) {
        return false;  // 周末
    }
    
    // 检查时间是否在工作时间内 (9:00-18:00)
    if (time_info.hour >= 9 && time_info.hour < 18) {
        return true;
    }
    
    return false;
}

/**
 * @brief 获取时间差（秒）
 * @param start_time 开始时间戳
 * @param end_time 结束时间戳
 * @return int64_t 时间差（秒）
 */
int64_t get_time_diff_seconds(time_t start_time, time_t end_time)
{
    return (int64_t)(end_time - start_time);
}

/**
 * @brief 打印当前时间信息到日志
 */
void print_current_time(void)
{
    time_info_t time_info;
    if (get_current_time_info(&time_info) == ESP_OK) {
        ESP_LOGI(TAG, "当前时间: %d年%d月%d日 %02d:%02d:%02d %s", 
                 time_info.year, time_info.month, time_info.day,
                 time_info.hour, time_info.minute, time_info.second, 
                 weekdays[time_info.weekday]);
    } else {
        ESP_LOGE(TAG, "获取时间信息失败");
    }
}