#!/usr/bin/env python3
"""
喂食管理系统测试脚本
用于测试ESP32喂食管理系统的HTTP接口功能
"""

import requests
import json
import time
from datetime import datetime

# ESP32设备IP地址（需要根据实际情况修改）
ESP32_IP = "192.168.1.100"
BASE_URL = f"http://{ESP32_IP}"

def test_get_feeding_plans():
    """测试获取喂食计划列表"""
    print("=== 测试获取喂食计划列表 ===")
    try:
        response = requests.get(f"{BASE_URL}/api/feeding-plans")
        if response.status_code == 200:
            data = response.json()
            print(f"成功获取 {data['count']} 个喂食计划")
            for plan in data['plans']:
                print(f"  计划ID: {plan['id']}, 星期{plan['day_of_week']} {plan['hour']:02d}:{plan['minute']:02d}, "
                      f"喂食量: {plan['feeding_amount']}g, 启用: {plan['enabled']}")
        else:
            print(f"获取失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def test_add_feeding_plan():
    """测试添加喂食计划"""
    print("\n=== 测试添加喂食计划 ===")
    plan_data = {
        "day_of_week": 1,  # 星期一
        "hour": 8,
        "minute": 0,
        "feeding_amount": 50.0
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/feeding-plans",
            json=plan_data,
            headers={"Content-Type": "application/json"}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"添加成功: {data['message']}")
        else:
            print(f"添加失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def test_add_manual_feeding():
    """测试添加手动喂食"""
    print("\n=== 测试添加手动喂食 ===")
    # 设置5分钟后执行
    now = datetime.now()
    target_time = now.replace(second=0, microsecond=0)
    target_time = target_time.replace(minute=(target_time.minute + 5) % 60)
    if target_time.minute < 5:
        target_time = target_time.replace(hour=(target_time.hour + 1) % 24)
    
    feeding_data = {
        "hour": target_time.hour,
        "minute": target_time.minute,
        "feeding_amount": 30.0
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/manual-feeding",
            json=feeding_data,
            headers={"Content-Type": "application/json"}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"添加成功: {data['message']}")
            print(f"将在 {target_time.hour:02d}:{target_time.minute:02d} 执行喂食")
        else:
            print(f"添加失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def test_immediate_feeding():
    """测试立即喂食"""
    print("\n=== 测试立即喂食 ===")
    feeding_data = {
        "feeding_amount": 25.0
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/immediate-feeding",
            json=feeding_data,
            headers={"Content-Type": "application/json"}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"立即喂食成功: {data['message']}")
        else:
            print(f"立即喂食失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def test_get_manual_feedings():
    """测试获取手动喂食列表"""
    print("\n=== 测试获取手动喂食列表 ===")
    try:
        response = requests.get(f"{BASE_URL}/api/manual-feedings")
        if response.status_code == 200:
            data = response.json()
            print(f"成功获取 {data['count']} 个手动喂食")
            for feeding in data['feedings']:
                print(f"  喂食ID: {feeding['id']}, 时间: {feeding['hour']:02d}:{feeding['minute']:02d}, "
                      f"喂食量: {feeding['feeding_amount']}g, 已执行: {feeding['executed']}")
        else:
            print(f"获取失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def test_toggle_feeding_plan():
    """测试启用/禁用喂食计划"""
    print("\n=== 测试启用/禁用喂食计划 ===")
    toggle_data = {
        "id": 1,
        "enabled": False
    }
    
    try:
        response = requests.put(
            f"{BASE_URL}/api/feeding-plans/toggle",
            json=toggle_data,
            headers={"Content-Type": "application/json"}
        )
        if response.status_code == 200:
            data = response.json()
            print(f"操作成功: {data['message']}")
        else:
            print(f"操作失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def test_delete_feeding_plan():
    """测试删除喂食计划"""
    print("\n=== 测试删除喂食计划 ===")
    plan_id = 1
    
    try:
        response = requests.delete(f"{BASE_URL}/api/feeding-plans?id={plan_id}")
        if response.status_code == 200:
            data = response.json()
            print(f"删除成功: {data['message']}")
        else:
            print(f"删除失败，状态码: {response.status_code}")
    except Exception as e:
        print(f"请求失败: {e}")

def main():
    """主测试函数"""
    print("喂食管理系统测试脚本")
    print("=" * 50)
    
    # 检查ESP32连接
    try:
        response = requests.get(f"{BASE_URL}/api/feeding-plans", timeout=5)
        print("ESP32连接正常")
    except Exception as e:
        print(f"无法连接到ESP32: {e}")
        print("请检查:")
        print("1. ESP32是否已启动")
        print("2. IP地址是否正确")
        print("3. 网络连接是否正常")
        return
    
    # 执行测试
    test_get_feeding_plans()
    test_add_feeding_plan()
    test_add_manual_feeding()
    test_get_manual_feedings()
    test_immediate_feeding()
    test_toggle_feeding_plan()
    test_delete_feeding_plan()
    
    print("\n" + "=" * 50)
    print("测试完成")

if __name__ == "__main__":
    main() 