#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "feeding_manager.h"
#include "esp_websocket_client.h"

// Global WebSocket client handle
extern esp_websocket_client_handle_t g_ws_client;

esp_err_t feeding_plan_save_all_to_spiffs(void);
esp_err_t feeding_plan_load_all_from_spiffs(void);
int feeding_plan_add(const feeding_plan_t *plan);
int feeding_plan_delete(int index);
int feeding_plan_count(void);
const feeding_plan_t* feeding_plan_get(int index);
void feeding_plan_check_and_execute(void); // 定时调用，自动执行到点的计划
void feeding_plan_check_and_execute_simple(void); // 简化版本，用于定时器回调
void websocket_client_start(void); 

// 版本管理相关函数
bool websocket_has_available_update(void);
const char* websocket_get_latest_version(void);
const char* websocket_get_download_url(void);
void websocket_clear_update_status(void); 