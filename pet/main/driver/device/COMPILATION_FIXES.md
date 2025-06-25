# 设备管理器编译修复说明

## 编译错误及解决方案

### 1. 头文件包含错误

**错误信息:**
```
'device_manager.h' file not found
'wifi_status_manager.h' file not found
```

**解决方案:**
确保在 `main/CMakeLists.txt` 中正确添加了包含目录：

```cmake
idf_component_register(SRCS 
    "main.c" 
    "./WIFI/ConnectWIFI.c" 
    "./WIFI/wifi_status_manager.c"
    "./driver/device/device_manager.c"
    # 其他源文件...
    INCLUDE_DIRS 
    "." 
    "./WIFI"
    "./driver/device"
    # 其他包含目录...
)
```

### 2. 类型定义错误

**错误信息:**
```
Unknown type name 'bool'
Unknown type name 'heartbeat_response_callback_t'
```

**解决方案:**
在头文件中添加必要的包含：

```c
#include <stdbool.h>
#include "esp_err.h"
```

### 3. 函数声明错误

**错误信息:**
```
Call to undeclared function 'strncpy'
Call to undeclared function 'save_device_info_to_nvs'
```

**解决方案:**
在源文件中添加必要的头文件：

```c
#include <string.h>
#include <stdio.h>
```

### 4. 编译参数错误

**错误信息:**
```
Unknown argument '-mlongcalls'
Unknown argument '-mdisable-hardware-atomics'
```

**解决方案:**
这些是ESP-IDF的编译参数，通常不需要手动修改。确保使用正确的ESP-IDF版本。

## 完整的修复步骤

### 1. 更新 CMakeLists.txt

确保 `main/CMakeLists.txt` 包含以下内容：

```cmake
idf_component_register(SRCS "main.c" 
                            "./WIFI/ConnectWIFI.c" 
                            "./WIFI/wifi_status_manager.c"
                            "./dns_server/my_dns_server.c" 
                            "./web_server/webserver.c" 
                            "./driver/hx711/hx711.c" 
                            "./driver/motor/motor.c" 
                            "./driver/key/key.c" 
                            "./driver/ws2812/ws2812.c"
                            "./driver/audio/audio.c"
                            "./driver/spiffs/app_spiffs.c"
                            "./driver/audio/hal_i2s.c"
                            "./driver/app_sr/app_sr.c"
                            "./driver/app_url_encode/app_url_encode.c"
                            "./driver/http/http_tts.c"
                            "./driver/http/http_client.c"
                            "./driver/timer/app_timer.c"
                            "./driver/timer/time_utils.c"
                            "./driver/feeding/feeding_manager.c"
                            "./driver/feeding/feeding_http_handler.c"
                            "./driver/device/device_manager.c"
                            "./driver/mqtt_client/my_mqtt_client.c"
                            INCLUDE_DIRS "." "./WIFI" 
                            "./dns_server"
                            "./web_server"   
                            "./driver/hx711" 
                            "./driver/motor" 
                            "./driver/key" 
                            "./driver/ws2812" 
                            "./driver/audio" 
                            "./driver/spiffs" 
                            "./driver/app_sr" 
                            "./driver/app_url_encode" 
                            "./driver/http"
                            "./driver/timer"
                            "./driver/feeding"
                            "./driver/device"
                            "./driver/mqtt_client"
                            EMBED_FILES "./web_server/index.html"
                            EMBED_TXTFILES
                                    "tuya_root_cert.pem"
                            )
```

### 2. 修复头文件包含

在 `main/driver/device/device_manager.h` 中：

```c
#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明...

#ifdef __cplusplus
}
#endif

#endif // DEVICE_MANAGER_H
```

在 `main/WIFI/wifi_status_manager.h` 中：

```c
#ifndef WIFI_STATUS_MANAGER_H
#define WIFI_STATUS_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明...

#ifdef __cplusplus
}
#endif

#endif // WIFI_STATUS_MANAGER_H
```

### 3. 修复源文件包含

在 `main/driver/device/device_manager.c` 中：

```c
#include "device_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include "http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>
#include <stdio.h>
```

在 `main/WIFI/wifi_status_manager.c` 中：

```c
#include "wifi_status_manager.h"
#include "device_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <string.h>
```

### 4. 修复 main.c

在 `main/main.c` 中：

```c
#include "device_manager.h"
#include "wifi_status_manager.h"

// WiFi状态变化回调函数
static void wifi_status_callback(bool connected)
{
    ESP_LOGI("main", "WiFi status changed: %s", connected ? "connected" : "disconnected");
    device_manager_set_wifi_status(connected);
}

void app_main(void)
{
    // 初始化设备管理器
    if (device_manager_init() == ESP_OK) {
        ESP_LOGI("main", "设备管理器初始化成功");
    } else {
        ESP_LOGE("main", "设备管理器初始化失败");
    }
    
    // 初始化WiFi状态管理器
    if (wifi_status_manager_init(wifi_status_callback) == ESP_OK) {
        ESP_LOGI("main", "WiFi状态管理器初始化成功");
    } else {
        ESP_LOGE("main", "WiFi状态管理器初始化失败");
    }
    
    // 连接WiFi
    wifi_init_sta("adol-3466","12345678");
    
    // 其他初始化代码...
}
```

## 编译命令

```bash
# 清理之前的编译
idf.py clean

# 重新编译
idf.py build

# 如果编译成功，可以烧录到设备
idf.py flash monitor
```

## 测试步骤

### 1. 启动测试服务器

```bash
python3 test_device_heartbeat.py
```

### 2. 编译并烧录ESP32固件

```bash
idf.py build flash monitor
```

### 3. 观察日志输出

设备启动后应该看到类似以下的日志：

```
I (1234) DEVICE_MANAGER: Initializing device manager...
I (1235) DEVICE_MANAGER: Device not initialized, will request ID from server
I (1236) DEVICE_MANAGER: Device manager initialized successfully
I (2345) WIFI_STATUS: WiFi connected to AP
I (2346) DEVICE_MANAGER: WiFi status changed: connected
I (2347) DEVICE_MANAGER: Sending initial heartbeat...
I (3456) HTTP_CLIENT: Received new device ID: DEVICE_12345678
I (3457) DEVICE_MANAGER: Device initialized successfully
```

### 4. 检查服务器端

在服务器端应该看到：

```
[2024-01-01 12:00:00] 收到心跳包:
  设备ID: NULL
  设备类型: pet_feeder
  固件版本: 1.0.0
  分配新设备ID: DEVICE_12345678
  分配密码: PASS_ABCDEF
```

## 常见问题

### 1. 编译时找不到头文件

确保所有新创建的头文件都在正确的目录中，并且CMakeLists.txt中包含了正确的路径。

### 2. 链接错误

确保所有源文件都已在CMakeLists.txt的SRCS列表中。

### 3. 运行时错误

检查NVS是否正确初始化，WiFi连接是否成功。

### 4. 心跳包发送失败

检查服务器是否正在运行，网络连接是否正常。 