# 时间工具模块使用说明

## 概述
这个时间工具模块提供了获取当前时间、格式化时间字符串、计算时间差等功能，特别适用于ESP32项目中需要时间相关功能的场景。

## 主要功能

### 1. 获取当前时间信息
```c
time_info_t time_info;
esp_err_t ret = get_current_time_info(&time_info);
if (ret == ESP_OK) {
    printf("时间: %02d:%02d:%02d\n", time_info.hour, time_info.minute, time_info.second);
    printf("星期: %d\n", time_info.weekday); // 0=星期日, 1=星期一, ..., 6=星期六
    printf("日期: %d年%d月%d日\n", time_info.year, time_info.month, time_info.day);
}
```

### 2. 格式化时间字符串
```c
char time_str[64];

// 中文格式: 14:30:25 星期一
get_formatted_time_string(time_str, sizeof(time_str), 0);

// 英文格式: 14:30:25 Monday
get_formatted_time_string(time_str, sizeof(time_str), 1);

// 数字格式: 14:30:25 1
get_formatted_time_string(time_str, sizeof(time_str), 2);

// 完整中文格式: 2024年1月15日 14:30:25 星期一
get_formatted_time_string(time_str, sizeof(time_str), 3);
```

### 3. 获取时间戳
```c
time_t timestamp = get_current_timestamp();
printf("当前时间戳: %lld\n", (long long)timestamp);
```

### 4. 检查工作时间
```c
if (is_work_time()) {
    printf("现在是工作时间 (周一到周五 9:00-18:00)\n");
} else {
    printf("现在不是工作时间\n");
}
```

### 5. 计算时间差
```c
time_t start_time = get_current_timestamp();
// ... 执行一些操作 ...
time_t end_time = get_current_timestamp();
int64_t diff = get_time_diff_seconds(start_time, end_time);
printf("耗时: %lld 秒\n", (long long)diff);
```

### 6. 打印当前时间到日志
```c
print_current_time(); // 输出到ESP_LOG
```

## 数据结构

### time_info_t 结构体
```c
typedef struct {
    int hour;       // 小时 (0-23)
    int minute;     // 分钟 (0-59)
    int second;     // 秒 (0-59)
    int weekday;    // 星期 (0=星期日, 1=星期一, ..., 6=星期六)
    int year;       // 年份
    int month;      // 月份 (1-12)
    int day;        // 日期 (1-31)
} time_info_t;
```

## 使用示例

### 基本使用
```c
#include "time_utils.h"

void app_main(void) {
    // 确保已经同步了网络时间
    sync_time_sntp();
    
    // 获取当前时间
    time_info_t time_info;
    if (get_current_time_info(&time_info) == ESP_OK) {
        printf("现在是 %02d:%02d:%02d %s\n", 
               time_info.hour, time_info.minute, time_info.second,
               "星期日\0星期一\0星期二\0星期三\0星期四\0星期五\0星期六" + time_info.weekday * 9);
    }
}
```

### 在定时器中使用
```c
#include "time_utils.h"
#include "esp_timer.h"

void timer_callback(void* arg) {
    char time_str[32];
    if (get_formatted_time_string(time_str, sizeof(time_str), 0) == ESP_OK) {
        printf("定时器触发时间: %s\n", time_str);
    }
}
```

## 注意事项

1. **时间同步**: 使用前确保已经通过SNTP同步了网络时间
2. **内存安全**: 使用格式化函数时注意缓冲区大小
3. **时区设置**: 默认使用系统时区，如需修改请配置ESP32的时区设置
4. **网络依赖**: 时间同步需要网络连接

## 编译配置

确保在 `main/CMakeLists.txt` 中包含了 `time_utils.c` 文件：

```cmake
idf_component_register(SRCS 
    "main.c"
    "./driver/timer/time_utils.c"
    # ... 其他源文件
    INCLUDE_DIRS 
    "."
    "./driver/timer"
    # ... 其他包含目录
)
```

## 错误处理

所有函数都返回 `esp_err_t` 类型，使用前请检查返回值：

```c
esp_err_t ret = get_current_time_info(&time_info);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "获取时间信息失败: %s", esp_err_to_name(ret));
    return;
}
``` 