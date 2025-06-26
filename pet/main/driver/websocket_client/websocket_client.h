#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "feeding_manager.h"

esp_err_t feeding_plan_save_all_to_spiffs(void);
esp_err_t feeding_plan_load_all_from_spiffs(void);
int feeding_plan_add(const feeding_plan_t *plan);
int feeding_plan_delete(int index);
int feeding_plan_count(void);
const feeding_plan_t* feeding_plan_get(int index);
void feeding_plan_check_and_execute(void); // 定时调用，自动执行到点的计划
void websocket_client_start(void); 