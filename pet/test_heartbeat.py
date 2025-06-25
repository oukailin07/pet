#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试心跳包API
"""

import requests
import json
import time

def test_heartbeat():
    """测试心跳包API"""
    url = "http://127.0.0.1:80/device/heartbeat"
    
    # 模拟新设备（没有device_id）
    data = {
        "device_id": "",  # 空字符串表示新设备
        "device_type": "pet_feeder",
        "firmware_version": "1.0.0"
    }
    
    print("发送心跳包...")
    print(f"URL: {url}")
    print(f"数据: {json.dumps(data, indent=2)}")
    
    try:
        response = requests.post(url, json=data, timeout=10)
        print(f"状态码: {response.status_code}")
        print(f"响应: {response.text}")
        
        if response.status_code == 200:
            result = response.json()
            if result.get("need_device_id"):
                print(f"✅ 成功分配设备ID: {result.get('device_id')}")
                print(f"✅ 设备密码: {result.get('password')}")
            else:
                print("✅ 心跳包发送成功")
        else:
            print("❌ 心跳包发送失败")
            
    except Exception as e:
        print(f"❌ 请求失败: {e}")

if __name__ == "__main__":
    print("=== 宠物喂食器心跳包测试 ===")
    test_heartbeat()
    print("=== 测试完成 ===") 