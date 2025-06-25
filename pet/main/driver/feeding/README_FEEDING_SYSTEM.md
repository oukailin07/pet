# 喂食管理系统使用说明

## 概述
这是一个完整的智能宠物喂食管理系统，支持定时喂食计划、手动喂食、立即喂食等功能。系统会自动监控时间，在指定时间执行喂食操作，并通过重量传感器控制喂食量。

## 主要功能

### 1. 定时喂食计划
- 支持设置每周不同时间的喂食计划
- 每个计划包含：星期、时间、喂食量
- 可以启用/禁用单个计划
- 计划保存在SPIFFS中，重启后自动加载

### 2. 手动喂食
- 支持设置延迟执行的手动喂食
- 执行完成后自动删除
- 适合临时调整喂食时间

### 3. 立即喂食
- 支持立即执行喂食操作
- 适合紧急喂食需求

### 4. 自动重量控制
- 通过HX711重量传感器精确控制喂食量
- 实时监控重量变化
- 达到目标重量后自动停止

### 5. 数据上报
- 自动上报喂食记录到服务器
- 支持手动喂食记录上报
- 记录包含时间、喂食量等信息

## 系统架构

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   HTTP接口      │    │   喂食管理器    │    │   硬件控制      │
│                 │    │                 │    │                 │
│ - 获取计划列表  │◄──►│ - 计划管理      │◄──►│ - 电机控制      │
│ - 添加计划      │    │ - 时间监控      │    │ - 重量传感器    │
│ - 删除计划      │    │ - 自动执行      │    │ - LED指示灯     │
│ - 立即喂食      │    │ - 数据上报      │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   SPIFFS存储    │    │   时间工具      │    │   网络通信      │
│                 │    │                 │    │                 │
│ - 喂食计划      │    │ - 时间获取      │    │ - HTTP客户端    │
│ - 手动喂食      │    │ - 时间格式化    │    │ - 数据上报      │
│ - 喂食记录      │    │ - 星期判断      │    │ - 服务器通信    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## API接口说明

### 1. 获取喂食计划列表
```
GET /api/feeding-plans
```
响应示例：
```json
{
  "plans": [
    {
      "id": 1,
      "day_of_week": 1,
      "hour": 8,
      "minute": 0,
      "feeding_amount": 50.0,
      "enabled": true
    }
  ],
  "count": 1
}
```

### 2. 添加喂食计划
```
POST /api/feeding-plans
Content-Type: application/json

{
  "day_of_week": 1,
  "hour": 8,
  "minute": 0,
  "feeding_amount": 50.0
}
```

### 3. 删除喂食计划
```
DELETE /api/feeding-plans?id=1
```

### 4. 启用/禁用喂食计划
```
PUT /api/feeding-plans/toggle
Content-Type: application/json

{
  "id": 1,
  "enabled": false
}
```

### 5. 添加手动喂食
```
POST /api/manual-feeding
Content-Type: application/json

{
  "hour": 14,
  "minute": 30,
  "feeding_amount": 30.0
}
```

### 6. 立即喂食
```
POST /api/immediate-feeding
Content-Type: application/json

{
  "feeding_amount": 25.0
}
```

### 7. 获取手动喂食列表
```
GET /api/manual-feedings
```

## 使用示例

### 1. 基本使用流程

```c
#include "feeding_manager.h"

void app_main(void) {
    // 初始化系统
    feeding_manager_init();
    
    // 添加定时喂食计划
    add_feeding_plan(1, 8, 0, 50.0);   // 星期一 8:00 喂食50g
    add_feeding_plan(1, 18, 0, 50.0);  // 星期一 18:00 喂食50g
    
    // 添加手动喂食
    add_manual_feeding(14, 30, 30.0);  // 今天 14:30 喂食30g
    
    // 立即喂食
    execute_immediate_feeding(25.0);   // 立即喂食25g
}
```

### 2. 通过HTTP接口使用

```bash
# 添加定时喂食计划
curl -X POST http://192.168.1.100/api/feeding-plans \
  -H "Content-Type: application/json" \
  -d '{"day_of_week": 1, "hour": 8, "minute": 0, "feeding_amount": 50.0}'

# 立即喂食
curl -X POST http://192.168.1.100/api/immediate-feeding \
  -H "Content-Type: application/json" \
  -d '{"feeding_amount": 25.0}'

# 获取计划列表
curl http://192.168.1.100/api/feeding-plans
```

## 配置文件

### 1. 设备配置
在 `feeding_manager.c` 中可以修改以下配置：

```c
// 最大喂食计划数量
#define MAX_FEEDING_PLANS 20
#define MAX_MANUAL_FEEDINGS 10

// 设备ID
static char device_id[32] = "ESP32_PET_001";
```

### 2. 文件路径
```c
#define FEEDING_PLANS_FILE "/spiffs/feeding_plans.json"
#define MANUAL_FEEDING_FILE "/spiffs/manual_feeding.json"
#define FEEDING_RECORDS_FILE "/spiffs/feeding_records.json"
```

## 工作流程

### 1. 系统启动流程
1. 初始化SPIFFS文件系统
2. 加载保存的喂食计划
3. 加载保存的手动喂食
4. 启动时间同步
5. 启动喂食管理任务

### 2. 定时喂食执行流程
1. 每分钟检查当前时间
2. 匹配启用的喂食计划
3. 启动电机开始出粮
4. 实时监控重量变化
5. 达到目标重量后停止电机
6. 上报喂食记录到服务器

### 3. 手动喂食执行流程
1. 检查手动喂食时间
2. 执行喂食操作
3. 上报手动喂食记录
4. 删除已执行的手动喂食

## 错误处理

### 1. 常见错误及解决方案

| 错误 | 原因 | 解决方案 |
|------|------|----------|
| 重量传感器读取失败 | 传感器连接问题 | 检查HX711接线 |
| 电机不转动 | 电机驱动问题 | 检查电机接线和驱动 |
| 时间同步失败 | 网络连接问题 | 检查WiFi连接 |
| SPIFFS写入失败 | 存储空间不足 | 清理不必要的文件 |

### 2. 调试信息
系统会输出详细的调试日志，包括：
- 喂食计划加载状态
- 时间检查过程
- 电机控制状态
- 重量监控数据
- 网络通信状态

## 安全注意事项

1. **重量校准**: 首次使用前需要校准重量传感器
2. **电机保护**: 避免长时间连续运行电机
3. **网络安全**: 建议使用HTTPS进行数据传输
4. **数据备份**: 定期备份喂食计划数据

## 扩展功能

### 1. 可扩展的功能
- 多宠物支持
- 喂食历史统计
- 智能喂食算法
- 远程监控
- 语音控制

### 2. 硬件扩展
- 摄像头监控
- 温湿度传感器
- 水位检测
- 电池电量监控

## 技术支持

如果遇到问题，请检查：
1. 硬件连接是否正确
2. 网络连接是否正常
3. 时间同步是否成功
4. 日志输出中的错误信息

系统设计为模块化架构，便于维护和扩展。每个功能模块都有清晰的接口定义，可以根据需要进行定制和优化。 