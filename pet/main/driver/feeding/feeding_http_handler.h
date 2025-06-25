#ifndef FEEDING_HTTP_HANDLER_H
#define FEEDING_HTTP_HANDLER_H

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册喂食相关的HTTP接口处理器
 * @param server HTTP服务器句柄
 * @return esp_err_t ESP_OK成功，ESP_FAIL失败
 */
esp_err_t register_feeding_http_handlers(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif // FEEDING_HTTP_HANDLER_H 