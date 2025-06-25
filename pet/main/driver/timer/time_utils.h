 #ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 时间信息结构体
 */
typedef struct {
    int hour;       // 小时 (0-23)
    int minute;     // 分钟 (0-59)
    int second;     // 秒 (0-59)
    int weekday;    // 星期 (0=星期日, 1=星期一, ..., 6=星期六)
    int year;       // 年份
    int month;      // 月份 (1-12)
    int day;        // 日期 (1-31)
} time_info_t;

/**
 * @brief 获取当前时间的时分秒和星期
 * @param time_info 时间信息结构体指针
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t get_current_time_info(time_info_t *time_info);

/**
 * @brief 获取格式化的时间字符串
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @param format 格式类型 (0: 中文格式, 1: 英文格式, 2: 数字格式, 3: 完整中文格式)
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t get_formatted_time_string(char *buffer, size_t buffer_size, int format);

/**
 * @brief 获取当前时间戳
 * @return time_t 当前时间戳
 */
time_t get_current_timestamp(void);

/**
 * @brief 检查是否为工作时间 (9:00-18:00)
 * @return bool true为工作时间，false为非工作时间
 */
bool is_work_time(void);

/**
 * @brief 获取时间差（秒）
 * @param start_time 开始时间戳
 * @param end_time 结束时间戳
 * @return int64_t 时间差（秒）
 */
int64_t get_time_diff_seconds(time_t start_time, time_t end_time);

/**
 * @brief 打印当前时间信息到日志
 */
void print_current_time(void);

#ifdef __cplusplus
}
#endif

#endif // TIME_UTILS_H