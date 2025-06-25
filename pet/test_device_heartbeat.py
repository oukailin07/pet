 #!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
设备心跳包测试服务器
模拟服务器端处理ESP32设备的心跳包请求
"""

from flask import Flask, request, jsonify
import json
import uuid
import time
from datetime import datetime
import threading

app = Flask(__name__)

# 存储设备信息的字典
devices = {}

# 设备ID生成器
def generate_device_id():
    """生成唯一的设备ID"""
    return f"DEVICE_{uuid.uuid4().hex[:8].upper()}"

# 密码生成器
def generate_password():
    """生成设备密码"""
    return f"PASS_{uuid.uuid4().hex[:6].upper()}"

@app.route('/device/heartbeat', methods=['POST'])
def device_heartbeat():
    """处理设备心跳包"""
    try:
        # 获取请求数据
        data = request.get_json()
        if not data:
            return jsonify({"error": "Invalid JSON data"}), 400
        
        device_id = data.get('device_id', '')
        device_type = data.get('device_type', 'unknown')
        firmware_version = data.get('firmware_version', 'unknown')
        
        print(f"[{datetime.now()}] 收到心跳包:")
        print(f"  设备ID: {device_id if device_id else 'NULL'}")
        print(f"  设备类型: {device_type}")
        print(f"  固件版本: {firmware_version}")
        
        # 检查设备是否已注册
        if not device_id or device_id not in devices:
            # 新设备，分配设备ID和密码
            new_device_id = generate_device_id()
            new_password = generate_password()
            
            # 保存设备信息
            devices[new_device_id] = {
                'password': new_password,
                'device_type': device_type,
                'firmware_version': firmware_version,
                'first_seen': time.time(),
                'last_seen': time.time(),
                'heartbeat_count': 1
            }
            
            print(f"  分配新设备ID: {new_device_id}")
            print(f"  分配密码: {new_password}")
            
            response = {
                "need_device_id": True,
                "device_id": new_device_id,
                "password": new_password,
                "message": "Device registered successfully"
            }
        else:
            # 已注册设备，更新最后心跳时间
            devices[device_id]['last_seen'] = time.time()
            devices[device_id]['heartbeat_count'] += 1
            
            print(f"  设备已注册，心跳计数: {devices[device_id]['heartbeat_count']}")
            
            response = {
                "need_device_id": False,
                "status": "success",
                "message": "Heartbeat received",
                "heartbeat_count": devices[device_id]['heartbeat_count']
            }
        
        return jsonify(response), 200
        
    except Exception as e:
        print(f"处理心跳包时出错: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/devices', methods=['GET'])
def list_devices():
    """列出所有注册的设备"""
    device_list = []
    current_time = time.time()
    
    for device_id, info in devices.items():
        # 计算设备在线时长
        online_duration = current_time - info['first_seen']
        last_seen_duration = current_time - info['last_seen']
        
        device_info = {
            'device_id': device_id,
            'device_type': info['device_type'],
            'firmware_version': info['firmware_version'],
            'heartbeat_count': info['heartbeat_count'],
            'online_duration_minutes': int(online_duration / 60),
            'last_seen_minutes_ago': int(last_seen_duration / 60),
            'is_online': last_seen_duration < 300  # 5分钟内有心跳认为在线
        }
        device_list.append(device_info)
    
    return jsonify({
        "total_devices": len(device_list),
        "devices": device_list
    }), 200

@app.route('/device/<device_id>', methods=['GET'])
def get_device_info(device_id):
    """获取特定设备的详细信息"""
    if device_id not in devices:
        return jsonify({"error": "Device not found"}), 404
    
    info = devices[device_id]
    current_time = time.time()
    
    device_info = {
        'device_id': device_id,
        'password': info['password'],
        'device_type': info['device_type'],
        'firmware_version': info['firmware_version'],
        'heartbeat_count': info['heartbeat_count'],
        'first_seen': datetime.fromtimestamp(info['first_seen']).strftime('%Y-%m-%d %H:%M:%S'),
        'last_seen': datetime.fromtimestamp(info['last_seen']).strftime('%Y-%m-%d %H:%M:%S'),
        'online_duration_minutes': int((current_time - info['first_seen']) / 60),
        'last_seen_minutes_ago': int((current_time - info['last_seen']) / 60),
        'is_online': (current_time - info['last_seen']) < 300
    }
    
    return jsonify(device_info), 200

@app.route('/device/<device_id>', methods=['DELETE'])
def delete_device(device_id):
    """删除设备"""
    if device_id not in devices:
        return jsonify({"error": "Device not found"}), 404
    
    del devices[device_id]
    return jsonify({"message": f"Device {device_id} deleted successfully"}), 200

@app.route('/stats', methods=['GET'])
def get_stats():
    """获取统计信息"""
    current_time = time.time()
    online_devices = 0
    total_heartbeats = 0
    
    for device_id, info in devices.items():
        if (current_time - info['last_seen']) < 300:  # 5分钟内有心跳
            online_devices += 1
        total_heartbeats += info['heartbeat_count']
    
    stats = {
        "total_devices": len(devices),
        "online_devices": online_devices,
        "offline_devices": len(devices) - online_devices,
        "total_heartbeats": total_heartbeats,
        "server_uptime_minutes": int((current_time - app.start_time) / 60) if hasattr(app, 'start_time') else 0
    }
    
    return jsonify(stats), 200

@app.route('/', methods=['GET'])
def index():
    """服务器首页"""
    return jsonify({
        "message": "Device Heartbeat Server",
        "version": "1.0.0",
        "endpoints": {
            "POST /device/heartbeat": "设备心跳包",
            "GET /devices": "列出所有设备",
            "GET /device/<device_id>": "获取设备详情",
            "DELETE /device/<device_id>": "删除设备",
            "GET /stats": "获取统计信息"
        }
    }), 200

def cleanup_offline_devices():
    """清理离线设备（超过1小时无心跳）"""
    while True:
        time.sleep(60)  # 每分钟检查一次
        current_time = time.time()
        offline_devices = []
        
        for device_id, info in devices.items():
            if (current_time - info['last_seen']) > 3600:  # 1小时
                offline_devices.append(device_id)
        
        for device_id in offline_devices:
            print(f"清理离线设备: {device_id}")
            del devices[device_id]

if __name__ == '__main__':
    # 记录服务器启动时间
    app.start_time = time.time()
    
    # 启动清理线程
    cleanup_thread = threading.Thread(target=cleanup_offline_devices, daemon=True)
    cleanup_thread.start()
    
    print("设备心跳包测试服务器启动...")
    print("服务器地址: http://127.0.0.1:80")
    print("按 Ctrl+C 停止服务器")
    
    try:
        app.run(host='127.0.0.1', port=80, debug=False)
    except KeyboardInterrupt:
        print("\n服务器已停止")