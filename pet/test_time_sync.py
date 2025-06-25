#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
时间同步测试脚本
用于验证ESP32的时间同步功能
"""

import time
import datetime
import requests
import json

def test_ntp_servers():
    """测试NTP服务器是否可用"""
    ntp_servers = [
        "ntp.aliyun.com",
        "ntp1.aliyun.com", 
        "pool.ntp.org",
        "time.windows.com",
        "time.apple.com"
    ]
    
    print("测试NTP服务器可用性:")
    for server in ntp_servers:
        try:
            # 这里只是测试DNS解析，实际NTP查询需要专门的库
            import socket
            socket.gethostbyname(server)
            print(f"  ✓ {server} - DNS解析成功")
        except Exception as e:
            print(f"  ✗ {server} - DNS解析失败: {e}")
    
    print()

def get_current_time_info():
    """获取当前时间信息"""
    now = datetime.datetime.now()
    
    print("当前系统时间:")
    print(f"  本地时间: {now.strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"  UTC时间: {now.utcnow().strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"  时区: {time.tzname[time.daylight]}")
    print(f"  时间戳: {int(time.time())}")
    print(f"  星期: {now.strftime('%A')} ({now.weekday()})")
    print()

def test_timezone_conversion():
    """测试时区转换"""
    print("时区转换测试:")
    
    # 获取当前UTC时间
    utc_now = datetime.datetime.utcnow()
    print(f"  UTC时间: {utc_now.strftime('%Y-%m-%d %H:%M:%S')}")
    
    # 转换为北京时间 (UTC+8)
    beijing_time = utc_now + datetime.timedelta(hours=8)
    print(f"  北京时间: {beijing_time.strftime('%Y-%m-%d %H:%M:%S')}")
    
    # 转换为美国东部时间 (UTC-5)
    eastern_time = utc_now - datetime.timedelta(hours=5)
    print(f"  美国东部时间: {eastern_time.strftime('%Y-%m-%d %H:%M:%S')}")
    print()

def check_esp32_time_sync():
    """检查ESP32时间同步状态"""
    print("ESP32时间同步检查:")
    
    # 这里可以添加通过HTTP API检查ESP32时间的代码
    # 假设ESP32有一个时间查询接口
    try:
        # 示例：通过HTTP获取ESP32时间
        # response = requests.get('http://192.168.1.100/time', timeout=5)
        # esp32_time = response.json()
        # print(f"  ESP32时间: {esp32_time}")
        print("  注意: 需要ESP32提供时间查询接口")
    except Exception as e:
        print(f"  无法获取ESP32时间: {e}")
    
    print()

def generate_time_test_commands():
    """生成时间测试命令"""
    print("ESP32时间测试命令:")
    print()
    
    # 编译和烧录命令
    print("1. 编译和烧录:")
    print("   idf.py build")
    print("   idf.py flash monitor")
    print()
    
    # 观察日志的命令
    print("2. 观察时间同步日志:")
    print("   在串口监视器中应该看到:")
    print("   I (xxxx) SNTP: 开始时间同步...")
    print("   I (xxxx) SNTP: 等待时间同步... (1/15)")
    print("   I (xxxx) SNTP: 时间同步成功!")
    print("   I (xxxx) SNTP: 当前时间: 2024-01-15 14:30:25")
    print()
    
    # 手动时间查询命令
    print("3. 手动查询时间:")
    print("   在ESP32控制台中输入:")
    print("   print_current_time()")
    print()

def check_common_time_issues():
    """检查常见的时间问题"""
    print("常见时间问题检查:")
    
    # 检查系统时间是否合理
    now = datetime.datetime.now()
    if now.year < 2020:
        print("  ✗ 系统年份异常，可能时间未同步")
    else:
        print("  ✓ 系统年份正常")
    
    # 检查时区设置
    if time.tzname[time.daylight] in ['UTC', 'GMT']:
        print("  ⚠ 系统时区为UTC，可能需要设置本地时区")
    else:
        print("  ✓ 系统时区已设置")
    
    # 检查网络连接
    try:
        requests.get('http://www.baidu.com', timeout=5)
        print("  ✓ 网络连接正常")
    except:
        print("  ✗ 网络连接异常，可能影响时间同步")
    
    print()

def main():
    """主函数"""
    print("=" * 50)
    print("ESP32时间同步测试工具")
    print("=" * 50)
    print()
    
    # 获取当前时间信息
    get_current_time_info()
    
    # 检查常见问题
    check_common_time_issues()
    
    # 测试时区转换
    test_timezone_conversion()
    
    # 测试NTP服务器
    test_ntp_servers()
    
    # 生成测试命令
    generate_time_test_commands()
    
    # 检查ESP32时间同步
    check_esp32_time_sync()
    
    print("=" * 50)
    print("测试完成")
    print("=" * 50)

if __name__ == "__main__":
    main() 