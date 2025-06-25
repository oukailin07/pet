#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试ESP32心跳包API
"""

import requests
import json
import time

def test_esp32_heartbeat():
    """测试ESP32心跳包API"""
    url = "http://192.168.0.101:80/device/heartbeat"
    
    # 模拟ESP32发送的心跳包（没有device_id）
    data = {
        "device_id": "",  # 空字符串表示新设备
        "device_type": "pet_feeder",
        "firmware_version": "1.0.0"
    }
    
    print("=== 测试ESP32心跳包 ===")
    print(f"目标服务器: {url}")
    print(f"ESP32 IP: 192.168.0.100")
    print(f"服务器IP: 192.168.0.101")
    print(f"发送数据: {json.dumps(data, indent=2)}")
    
    try:
        response = requests.post(url, json=data, timeout=10)
        print(f"状态码: {response.status_code}")
        print(f"响应内容: {response.text}")
        
        if response.status_code == 200:
            result = response.json()
            if result.get("need_device_id"):
                print(f"✅ 成功分配设备ID: {result.get('device_id')}")
                print(f"✅ 设备密码: {result.get('password')}")
                print("✅ ESP32应该能收到这些信息并保存到NVS")
            else:
                print("✅ 心跳包发送成功")
        else:
            print("❌ 心跳包发送失败")
            
    except requests.exceptions.ConnectionError:
        print("❌ 连接失败：无法连接到服务器")
        print("   请检查：")
        print("   1. 服务器是否在 pet_server 目录下运行")
        print("   2. 服务器是否监听在 0.0.0.0:80")
        print("   3. 防火墙是否允许80端口")
    except requests.exceptions.Timeout:
        print("❌ 请求超时")
    except Exception as e:
        print(f"❌ 请求失败: {e}")

if __name__ == "__main__":
    test_esp32_heartbeat()
    print("\n=== 测试完成 ===")
    print("如果测试成功，ESP32应该能看到类似日志：")
    print("I (xxxx) HTTP_CLIENT: Received new device ID: ESP-001")
    print("I (xxxx) DEVICE_MANAGER: Device initialized successfully") 