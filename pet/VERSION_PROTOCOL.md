# 宠物喂食器版本协议规范

## 1. 概述

本文档定义了宠物喂食器项目的版本管理协议，包括固件版本、通信协议版本、OTA升级机制等规范。

## 2. 版本号格式

### 2.1 固件版本号格式
```
v<major>.<minor>.<patch>[-<build>][-<suffix>]
```

**格式说明：**
- `major`: 主版本号，重大功能变更或架构调整时递增
- `minor`: 次版本号，新功能添加时递增
- `patch`: 补丁版本号，Bug修复时递增
- `build`: 构建号（可选），表示构建次数
- `suffix`: 后缀标识（可选），如 `alpha`、`beta`、`rc`、`stable`

**示例：**
- `v1.0.0` - 第一个稳定版本
- `v1.2.3` - 1.2版本的第三个补丁
- `v2.0.0-beta1` - 2.0版本的第一个测试版
- `v1.5.2-20231201` - 带构建日期的版本

### 2.2 通信协议版本格式
```
<major>.<minor>
```

**格式说明：**
- `major`: 主版本号，协议不兼容变更时递增
- `minor`: 次版本号，协议兼容性扩展时递增

**示例：**
- `1.0` - 初始协议版本
- `1.1` - 协议扩展版本
- `2.0` - 不兼容的协议更新

## 3. 版本管理策略

### 3.1 版本号递增规则

#### 主版本号 (major)
- 重大功能变更
- 架构重构
- 不兼容的API变更
- 通信协议不兼容变更

#### 次版本号 (minor)
- 新功能添加
- 通信协议兼容性扩展
- 性能优化
- 硬件支持扩展

#### 补丁版本号 (patch)
- Bug修复
- 安全补丁
- 小功能改进
- 文档更新

### 3.2 版本兼容性

#### 固件兼容性
- 同一主版本号内：完全兼容
- 不同主版本号：可能不兼容，需要特殊处理

#### 通信协议兼容性
- 客户端版本 >= 服务器版本：完全兼容
- 客户端版本 < 服务器版本：需要升级客户端

## 4. 版本信息定义

### 4.1 固件版本信息结构

```c
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
```

### 4.2 通信协议版本信息

```c
typedef struct {
    uint8_t major;           // 协议主版本号
    uint8_t minor;           // 协议次版本号
    char description[64];    // 协议描述
} protocol_version_t;
```

## 5. OTA升级协议

### 5.1 升级流程

1. **版本检查**
   - 设备启动时向服务器报告当前版本
   - 服务器检查是否有可用更新

2. **升级通知**
   - 服务器发送升级指令
   - 包含新版本信息和下载URL

3. **固件下载**
   - 设备从指定URL下载固件
   - 支持断点续传和校验

4. **固件验证**
   - 校验固件完整性
   - 验证版本兼容性

5. **固件安装**
   - 写入OTA分区
   - 设置启动分区
   - 重启设备

### 5.2 OTA消息格式

#### 升级检查请求
```json
{
    "type": "version_check",
    "device_id": "device_001",
    "firmware_version": "v1.2.3",
    "protocol_version": "1.0",
    "hardware_version": "v1.0"
}
```

#### 升级检查响应
```json
{
    "type": "version_check_result",
    "has_update": true,
    "latest_version": "v1.3.0",
    "download_url": "https://example.com/firmware/v1.3.0.bin",
    "file_size": 1048576,
    "checksum": "sha256:abc123...",
    "force_update": false,
    "description": "修复了喂食时间同步问题"
}
```

#### 升级指令
```json
{
    "type": "ota_update",
    "url": "https://example.com/firmware/v1.3.0.bin",
    "version": "v1.3.0",
    "checksum": "sha256:abc123...",
    "force": false
}
```

#### 升级状态报告
```json
{
    "type": "ota_status",
    "device_id": "device_001",
    "status": "downloading", // downloading, installing, success, failed
    "progress": 75,          // 下载进度百分比
    "error_code": 0,         // 错误代码，0表示无错误
    "error_message": ""      // 错误描述
}
```

## 6. 版本回滚机制

### 6.1 自动回滚
- 升级后启动失败时自动回滚
- 健康检查失败时触发回滚
- 保留最近两个可用版本

### 6.2 手动回滚
- 通过WebSocket指令触发回滚
- 支持指定回滚到特定版本

### 6.3 回滚消息格式
```json
{
    "type": "rollback_request",
    "target_version": "v1.2.3",
    "reason": "user_request"
}
```

## 7. 版本兼容性检查

### 7.1 固件兼容性检查
```c
bool check_firmware_compatibility(const firmware_version_t *current, 
                                 const firmware_version_t *target) {
    // 主版本号必须相同
    if (current->major != target->major) {
        return false;
    }
    
    // 目标版本不能低于当前版本
    if (target->minor < current->minor) {
        return false;
    }
    
    if (target->minor == current->minor && target->patch < current->patch) {
        return false;
    }
    
    return true;
}
```

### 7.2 协议兼容性检查
```c
bool check_protocol_compatibility(const protocol_version_t *client, 
                                 const protocol_version_t *server) {
    // 主版本号必须相同
    if (client->major != server->major) {
        return false;
    }
    
    // 客户端版本不能低于服务器版本
    if (client->minor < server->minor) {
        return false;
    }
    
    return true;
}
```

## 8. 版本信息存储

### 8.1 固件版本信息存储
- 存储在NVS分区
- 包含当前版本和备份版本信息
- 支持版本历史记录

### 8.2 版本信息结构
```c
typedef struct {
    firmware_version_t current_version;
    firmware_version_t backup_version;
    uint32_t install_time;
    uint32_t boot_count;
    bool is_stable;
} version_info_t;
```

## 9. 版本发布流程

### 9.1 开发版本
- 使用 `-alpha` 或 `-beta` 后缀
- 仅用于内部测试
- 不推送给生产设备

### 9.2 候选版本
- 使用 `-rc` 后缀
- 用于小范围测试
- 通过测试后可发布正式版

### 9.3 正式版本
- 无后缀或使用 `-stable` 后缀
- 经过充分测试
- 可推送给所有设备

## 10. 版本监控和统计

### 10.1 版本分布统计
- 统计各版本设备数量
- 监控版本升级成功率
- 跟踪版本稳定性指标

### 10.2 升级失败分析
- 记录升级失败原因
- 分析失败模式
- 优化升级流程

## 11. 安全考虑

### 11.1 固件签名
- 所有固件必须经过数字签名
- 验证固件来源和完整性
- 防止恶意固件安装

### 11.2 升级权限控制
- 限制升级频率
- 支持升级白名单
- 关键设备需要特殊授权

## 12. 实施指南

### 12.1 代码实现
1. 在 `main/version.h` 中定义版本信息
2. 在 `main/version.c` 中实现版本管理功能
3. 在WebSocket客户端中集成版本检查
4. 在OTA模块中实现版本验证

### 12.2 配置文件
1. 在 `CMakeLists.txt` 中定义版本宏
2. 在 `sdkconfig` 中配置版本相关选项
3. 在构建脚本中自动生成版本信息

### 12.3 测试验证
1. 单元测试版本管理功能
2. 集成测试OTA升级流程
3. 兼容性测试不同版本间通信

## 13. 变更记录

| 版本 | 日期 | 变更内容 | 作者 |
|------|------|----------|------|
| 1.0.0 | 2024-01-01 | 初始版本协议 | 开发团队 |

---

**注意：** 本协议版本为 v1.0.0，后续版本将在此文档基础上进行更新和完善。 