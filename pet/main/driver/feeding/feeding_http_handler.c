#include "feeding_http_handler.h"
#include "feeding_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "FEEDING_HTTP";

// HTTP响应函数
static esp_err_t send_json_response(httpd_req_t *req, cJSON *json, int status_code)
{
    char *json_string = cJSON_Print(json);
    if (json_string == NULL) {
        return ESP_FAIL;
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, status_code == 200 ? "200 OK" : "400 Bad Request");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    esp_err_t ret = httpd_resp_send(req, json_string, strlen(json_string));
    free(json_string);
    return ret;
}

// 处理OPTIONS请求（CORS预检）
static esp_err_t handle_options(httpd_req_t *req)
{
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// 获取所有喂食计划
static esp_err_t get_feeding_plans_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /api/feeding-plans");
    
    cJSON *root = cJSON_CreateObject();
    cJSON *plans_array = cJSON_CreateArray();
    
    int plan_count = get_feeding_plans_count();
    for (int i = 0; i < plan_count; i++) {
        feeding_plan_t *plan = get_feeding_plan(i);
        if (plan != NULL) {
            cJSON *plan_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(plan_obj, "id", plan->id);
            cJSON_AddNumberToObject(plan_obj, "day_of_week", plan->day_of_week);
            cJSON_AddNumberToObject(plan_obj, "hour", plan->hour);
            cJSON_AddNumberToObject(plan_obj, "minute", plan->minute);
            cJSON_AddNumberToObject(plan_obj, "feeding_amount", plan->feeding_amount);
            cJSON_AddBoolToObject(plan_obj, "enabled", plan->enabled);
            cJSON_AddItemToArray(plans_array, plan_obj);
        }
    }
    
    cJSON_AddItemToObject(root, "plans", plans_array);
    cJSON_AddNumberToObject(root, "count", plan_count);
    
    esp_err_t ret = send_json_response(req, root, 200);
    cJSON_Delete(root);
    return ret;
}

// 添加喂食计划
static esp_err_t add_feeding_plan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /api/feeding-plans");
    
    // 读取请求体
    char content[256];
    int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
    if (recv_len <= 0) {
        ESP_LOGE(TAG, "Failed to receive request body");
        return ESP_FAIL;
    }
    content[recv_len] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    // 提取参数
    cJSON *day_of_week = cJSON_GetObjectItem(json, "day_of_week");
    cJSON *hour = cJSON_GetObjectItem(json, "hour");
    cJSON *minute = cJSON_GetObjectItem(json, "minute");
    cJSON *feeding_amount = cJSON_GetObjectItem(json, "feeding_amount");
    
    if (!day_of_week || !hour || !minute || !feeding_amount) {
        ESP_LOGE(TAG, "Missing required parameters");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 添加喂食计划
    esp_err_t ret = add_feeding_plan(day_of_week->valueint, hour->valueint, 
                                    minute->valueint, feeding_amount->valuedouble);
    
    cJSON_Delete(json);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Feeding plan added successfully");
        return send_json_response(req, response, 200);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to add feeding plan");
        esp_err_t resp_ret = send_json_response(req, response, 400);
        cJSON_Delete(response);
        return resp_ret;
    }
}

// 删除喂食计划
static esp_err_t delete_feeding_plan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "DELETE /api/feeding-plans");
    
    // 从URL中获取计划ID
    char query[50];
    int recv_len = httpd_req_get_url_query_len(req);
    if (recv_len > 0) {
        recv_len = httpd_req_get_url_query_str(req, query, sizeof(query));
        if (recv_len > 0) {
            char param[10];
            if (httpd_query_key_value(query, "id", param, sizeof(param)) == ESP_OK) {
                int plan_id = atoi(param);
                esp_err_t ret = delete_feeding_plan(plan_id);
                
                cJSON *response = cJSON_CreateObject();
                if (ret == ESP_OK) {
                    cJSON_AddStringToObject(response, "status", "success");
                    cJSON_AddStringToObject(response, "message", "Feeding plan deleted successfully");
                    return send_json_response(req, response, 200);
                } else {
                    cJSON_AddStringToObject(response, "status", "error");
                    cJSON_AddStringToObject(response, "message", "Failed to delete feeding plan");
                    esp_err_t resp_ret = send_json_response(req, response, 400);
                    cJSON_Delete(response);
                    return resp_ret;
                }
            }
        }
    }
    
    // 参数错误
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "status", "error");
    cJSON_AddStringToObject(response, "message", "Invalid plan ID");
    esp_err_t ret = send_json_response(req, response, 400);
    cJSON_Delete(response);
    return ret;
}

// 启用/禁用喂食计划
static esp_err_t toggle_feeding_plan_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "PUT /api/feeding-plans/toggle");
    
    // 读取请求体
    char content[256];
    int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
    if (recv_len <= 0) {
        ESP_LOGE(TAG, "Failed to receive request body");
        return ESP_FAIL;
    }
    content[recv_len] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    // 提取参数
    cJSON *plan_id = cJSON_GetObjectItem(json, "id");
    cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    
    if (!plan_id || !enabled) {
        ESP_LOGE(TAG, "Missing required parameters");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 切换状态
    esp_err_t ret = enable_feeding_plan(plan_id->valueint, enabled->valueint);
    
    cJSON_Delete(json);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Feeding plan status updated");
        return send_json_response(req, response, 200);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to update feeding plan status");
        esp_err_t resp_ret = send_json_response(req, response, 400);
        cJSON_Delete(response);
        return resp_ret;
    }
}

// 添加手动喂食
static esp_err_t add_manual_feeding_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /api/manual-feeding");
    
    // 读取请求体
    char content[256];
    int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
    if (recv_len <= 0) {
        ESP_LOGE(TAG, "Failed to receive request body");
        return ESP_FAIL;
    }
    content[recv_len] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    // 提取参数
    cJSON *hour = cJSON_GetObjectItem(json, "hour");
    cJSON *minute = cJSON_GetObjectItem(json, "minute");
    cJSON *feeding_amount = cJSON_GetObjectItem(json, "feeding_amount");
    
    if (!hour || !minute || !feeding_amount) {
        ESP_LOGE(TAG, "Missing required parameters");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 添加手动喂食
    esp_err_t ret = add_manual_feeding(hour->valueint, minute->valueint, feeding_amount->valuedouble);
    
    cJSON_Delete(json);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Manual feeding added successfully");
        return send_json_response(req, response, 200);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to add manual feeding");
        esp_err_t resp_ret = send_json_response(req, response, 400);
        cJSON_Delete(response);
        return resp_ret;
    }
}

// 立即喂食
static esp_err_t immediate_feeding_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST /api/immediate-feeding");
    
    // 读取请求体
    char content[256];
    int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
    if (recv_len <= 0) {
        ESP_LOGE(TAG, "Failed to receive request body");
        return ESP_FAIL;
    }
    content[recv_len] = '\0';
    
    // 解析JSON
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    // 提取参数
    cJSON *feeding_amount = cJSON_GetObjectItem(json, "feeding_amount");
    
    if (!feeding_amount) {
        ESP_LOGE(TAG, "Missing feeding_amount parameter");
        cJSON_Delete(json);
        return ESP_FAIL;
    }
    
    // 执行立即喂食
    esp_err_t ret = execute_immediate_feeding(feeding_amount->valuedouble);
    
    cJSON_Delete(json);
    
    // 返回响应
    cJSON *response = cJSON_CreateObject();
    if (ret == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "message", "Immediate feeding executed successfully");
        return send_json_response(req, response, 200);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to execute immediate feeding");
        esp_err_t resp_ret = send_json_response(req, response, 400);
        cJSON_Delete(response);
        return resp_ret;
    }
}

// 获取手动喂食列表
static esp_err_t get_manual_feedings_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /api/manual-feedings");
    
    cJSON *root = cJSON_CreateObject();
    cJSON *feedings_array = cJSON_CreateArray();
    
    int feeding_count = get_manual_feedings_count();
    for (int i = 0; i < feeding_count; i++) {
        manual_feeding_t *feeding = get_manual_feeding(i);
        if (feeding != NULL) {
            cJSON *feeding_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(feeding_obj, "id", feeding->id);
            cJSON_AddNumberToObject(feeding_obj, "hour", feeding->hour);
            cJSON_AddNumberToObject(feeding_obj, "minute", feeding->minute);
            cJSON_AddNumberToObject(feeding_obj, "feeding_amount", feeding->feeding_amount);
            cJSON_AddBoolToObject(feeding_obj, "executed", feeding->executed);
            cJSON_AddItemToArray(feedings_array, feeding_obj);
        }
    }
    
    cJSON_AddItemToObject(root, "feedings", feedings_array);
    cJSON_AddNumberToObject(root, "count", feeding_count);
    
    esp_err_t ret = send_json_response(req, root, 200);
    cJSON_Delete(root);
    return ret;
}

// 注册HTTP路由
esp_err_t register_feeding_http_handlers(httpd_handle_t server)
{
    // 喂食计划相关接口
    httpd_uri_t get_plans_uri = {
        .uri = "/api/feeding-plans",
        .method = HTTP_GET,
        .handler = get_feeding_plans_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_plans_uri);
    
    httpd_uri_t add_plan_uri = {
        .uri = "/api/feeding-plans",
        .method = HTTP_POST,
        .handler = add_feeding_plan_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &add_plan_uri);
    
    httpd_uri_t delete_plan_uri = {
        .uri = "/api/feeding-plans",
        .method = HTTP_DELETE,
        .handler = delete_feeding_plan_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &delete_plan_uri);
    
    httpd_uri_t toggle_plan_uri = {
        .uri = "/api/feeding-plans/toggle",
        .method = HTTP_PUT,
        .handler = toggle_feeding_plan_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &toggle_plan_uri);
    
    // 手动喂食相关接口
    httpd_uri_t get_manual_uri = {
        .uri = "/api/manual-feedings",
        .method = HTTP_GET,
        .handler = get_manual_feedings_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_manual_uri);
    
    httpd_uri_t add_manual_uri = {
        .uri = "/api/manual-feeding",
        .method = HTTP_POST,
        .handler = add_manual_feeding_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &add_manual_uri);
    
    // 立即喂食接口
    httpd_uri_t immediate_uri = {
        .uri = "/api/immediate-feeding",
        .method = HTTP_POST,
        .handler = immediate_feeding_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &immediate_uri);
    
    // CORS预检接口
    httpd_uri_t options_uri = {
        .uri = "/api/*",
        .method = HTTP_OPTIONS,
        .handler = handle_options,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &options_uri);
    
    ESP_LOGI(TAG, "Feeding HTTP handlers registered successfully");
    return ESP_OK;
} 