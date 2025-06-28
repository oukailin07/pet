# 版本协议使用说明

本文档详细说明了如何使用宠物喂食器项目的版本管理功能。

## 1. 快速开始

### 1.1 构建项目
```bash
# 使用构建脚本（推荐）
./build_version.sh release

# 或直接使用ESP-IDF
idf.py build
```

### 1.2 发布新版本
```bash
# 发布补丁版本（1.0.0 -> 1.0.1）
./release_version.sh -p -t -b

# 发布次版本（1.0.0 -> 1.1.0）
./release_version.sh -m -t -b

# 发布主版本（1.0.0 -> 2.0.0）
./release_version.sh -M -t -b

# 发布指定版本
./release_version.sh v1.2.3 -t -b
```

## 2. 版本号规范

### 2.1 版本号格式
```
v<major>.<minor>.<patch>[-<build>][-<suffix>]
```

**示例：**
- `v1.0.0` - 第一个稳定版本
- `v1.2.3` - 1.2版本的第三个补丁
- `v2.0.0-beta1` - 2.0版本的第一个测试版
- `v1.5.2-20231201` - 带构建日期的版本

### 2.2 版本号递增规则

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

## 3. 构建脚本使用

### 3.1 build_version.sh
自动生成版本信息并构建固件。

```bash
# 构建发布版本
./build_version.sh release

# 构建调试版本
./build_version.sh debug

# 清理并构建
./build_version.sh release clean
```

**功能：**
- 自动生成版本信息头文件
- 获取Git提交信息
- 构建固件
- 生成版本报告

### 3.2 release_version.sh
自动化版本发布流程。

```bash
# 查看帮助
./release_version.sh --help

# 发布补丁版本
./release_version.sh -p -t -b

# 发布次版本并上传
./release_version.sh -m -t -b -u

# 发布指定版本
./release_version.sh v1.2.3 -t -b
```

**选项说明：**
- `-p, --patch`: 发布补丁版本
- `-m, --minor`: 发布次版本
- `-M, --major`: 发布主版本
- `-t, --tag`: 创建Git标签
- `-b, --build`: 构建固件
- `-u, --upload`: 上传到服务器

## 4. 版本管理API

### 4.1 获取版本信息
```c
#include "version.h"

// 获取固件版本
const firmware_version_t *fw_version = get_firmware_version();
char version_str[64];
get_version_string(fw_version, version_str, sizeof(version_str));
printf("固件版本: %s\n", version_str);

// 获取协议版本
const protocol_version_t *proto_version = get_protocol_version();
printf("协议版本: %d.%d\n", proto_version->major, proto_version->minor);

// 获取硬件版本
const hardware_version_t *hw_version = get_hardware_version();
printf("硬件版本: %d.%d\n", hw_version->major, hw_version->minor);
```

### 4.2 版本兼容性检查
```c
// 检查固件兼容性
firmware_version_t target = {1, 2, 0, 0, "stable"};
bool compatible = check_firmware_compatibility(get_firmware_version(), &target);

// 检查协议兼容性
protocol_version_t server = {1, 1, "Server Protocol"};
bool compatible = check_protocol_compatibility(get_protocol_version(), &server);
```

### 4.3 OTA升级
```c
// 获取OTA信息
ota_info_t ota_info;
get_ota_info(&ota_info);

// 更新OTA状态
update_ota_status(OTA_STATUS_DOWNLOADING, 50, 0, "");

// 版本回滚
rollback_version("v1.0.0");
```

## 5. WebSocket通信协议

### 5.1 版本检查
```json
// 客户端发送版本检查请求
{
    "type": "version_check",
    "device_id": "device_001",
    "firmware_version": "v1.0.0",
    "protocol_version": "1.0",
    "hardware_version": "1.0"
}

// 服务器响应
{
    "type": "version_check_result",
    "has_update": true,
    "latest_version": "v1.1.0",
    "download_url": "https://example.com/firmware/v1.1.0.bin",
    "file_size": 1048576,
    "checksum": "sha256:abc123...",
    "force_update": false,
    "description": "修复了喂食时间同步问题"
}
```

### 5.2 OTA升级
```json
// 服务器发送升级指令
{
    "type": "ota_update",
    "url": "https://example.com/firmware/v1.1.0.bin",
    "version": "v1.1.0",
    "checksum": "sha256:abc123...",
    "force": false
}

// 客户端报告升级状态
{
    "type": "ota_status",
    "device_id": "device_001",
    "status": "downloading",
    "progress": 75,
    "error_code": 0,
    "error_message": ""
}
```

### 5.3 版本回滚
```json
// 服务器发送回滚请求
{
    "type": "rollback_request",
    "target_version": "v1.0.0",
    "reason": "user_request"
}

// 客户端响应回滚结果
{
    "type": "rollback_result",
    "device_id": "device_001",
    "target_version": "v1.0.0",
    "success": true
}
```

## 6. 配置文件

### 6.1 main/version.h
定义版本号常量：
```c
#define FIRMWARE_VERSION_MAJOR    1
#define FIRMWARE_VERSION_MINOR    0
#define FIRMWARE_VERSION_PATCH    0
#define FIRMWARE_VERSION_BUILD    1
#define FIRMWARE_VERSION_SUFFIX   "stable"
```

### 6.2 main/version_info.h
自动生成的版本信息文件（由构建脚本生成）。

## 7. 最佳实践

### 7.1 版本发布流程
1. **开发阶段**
   - 使用 `-alpha` 或 `-beta` 后缀
   - 仅用于内部测试

2. **测试阶段**
   - 使用 `-rc` 后缀
   - 小范围测试

3. **发布阶段**
   - 使用 `-stable` 后缀或无后缀
   - 正式发布

### 7.2 版本兼容性
- 同一主版本号内保持完全兼容
- 不同主版本号需要特殊处理
- 协议版本遵循向后兼容原则

### 7.3 安全考虑
- 所有固件必须经过数字签名
- 验证固件来源和完整性
- 限制升级频率和权限

## 8. 故障排除

### 8.1 常见问题

**Q: 构建脚本执行失败**
A: 检查文件权限，确保脚本可执行：
```bash
chmod +x build_version.sh release_version.sh
```

**Q: 版本号解析错误**
A: 检查 `main/version.h` 文件格式是否正确。

**Q: OTA升级失败**
A: 检查网络连接、服务器状态和版本兼容性。

**Q: Git标签创建失败**
A: 确保有未提交的更改，或手动提交版本更新。

### 8.2 调试信息
启用详细日志：
```bash
# 构建时显示详细信息
./build_version.sh release 2>&1 | tee build.log

# 查看版本信息
grep "版本信息" build.log
```

## 9. 扩展功能

### 9.1 自定义版本检查
可以修改 `check_for_updates()` 函数来实现自定义的版本检查逻辑。

### 9.2 服务器集成
修改 `upload_to_server()` 函数来实现与特定服务器的集成。

### 9.3 版本统计
添加版本分布统计和升级成功率监控功能。

---

**注意：** 本使用说明基于版本协议 v1.0.0，后续版本可能会有更新。 