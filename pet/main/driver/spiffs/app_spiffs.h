#pragma once

#include "esp_spiffs.h"


esp_err_t app_spiffs_init();
esp_err_t bsp_spiffs_mount(void);