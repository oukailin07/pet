#include "time_utils.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TIME_EXAMPLE";

/**
 * @brief 时间工具使用示例任务
 */
void time_utils_example_task(void *pvParameters)
{
    ESP_LOGI(TAG, "开始时间工具示例...");
    
    while (1) {
        // 示例1: 获取基本时间信息
        time_info_t time_info;
        if (get_current_time_info(&time_info) == ESP_OK) {
            ESP_LOGI(TAG, "当前时间: %02d:%02d:%02d", 
                     time_info.hour, time_info.minute, time_info.second);
            ESP_LOGI(TAG, "当前日期: %d年%d月%d日", 
                     time_info.year, time_info.month, time_info.day);
            ESP_LOGI(TAG, "星期: %s", 
                     (time_info.weekday >= 0 && time_info.weekday < 7) ? 
                     "星期日\0星期一\0星期二\0星期三\0星期四\0星期五\0星期六" + time_info.weekday * 9 : "未知");
        }
        
        // 示例2: 获取格式化的时间字符串
        char time_str[64];
        
        // 中文格式
        if (get_formatted_time_string(time_str, sizeof(time_str), 0) == ESP_OK) {
            ESP_LOGI(TAG, "中文格式: %s", time_str);
        }
        
        // 英文格式
        if (get_formatted_time_string(time_str, sizeof(time_str), 1) == ESP_OK) {
            ESP_LOGI(TAG, "英文格式: %s", time_str);
        }
        
        // 数字格式
        if (get_formatted_time_string(time_str, sizeof(time_str), 2) == ESP_OK) {
            ESP_LOGI(TAG, "数字格式: %s", time_str);
        }
        
        // 完整中文格式
        if (get_formatted_time_string(time_str, sizeof(time_str), 3) == ESP_OK) {
            ESP_LOGI(TAG, "完整格式: %s", time_str);
        }
        
        // 示例3: 检查工作时间
        if (is_work_time()) {
            ESP_LOGI(TAG, "现在是工作时间");
        } else {
            ESP_LOGI(TAG, "现在不是工作时间");
        }
        
        // 示例4: 获取时间戳
        time_t timestamp = get_current_timestamp();
        ESP_LOGI(TAG, "当前时间戳: %lld", (long long)timestamp);
        
        // 示例5: 计算时间差
        time_t start_time = get_current_timestamp();
        vTaskDelay(pdMS_TO_TICKS(2000)); // 延迟2秒
        time_t end_time = get_current_timestamp();
        int64_t diff = get_time_diff_seconds(start_time, end_time);
        ESP_LOGI(TAG, "时间差: %lld 秒", (long long)diff);
        
        // 示例6: 打印当前时间
        print_current_time();
        
        ESP_LOGI(TAG, "----------------------------------------");
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒打印一次
    }
}

/**
 * @brief 启动时间工具示例
 */
void start_time_utils_example(void)
{
    xTaskCreate(time_utils_example_task, "time_example", 4096, NULL, 5, NULL);
} 