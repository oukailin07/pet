#pragma once

#include "esp_err.h"



esp_err_t app_sr_init();

/**
 * @brief crate app sr task
 *
 */
void app_sr_task_start(void);
