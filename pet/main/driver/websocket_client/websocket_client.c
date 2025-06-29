#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>
#include "websocket_client.h"
#include "esp_websocket_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "device_manager.h"
#include "time_utils.h" // 假设你有获取当前时间的API
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "version.h" // 添加版本管理头文件
#include "motor.h" // 添加电机头文件
#define WS_SERVER_URI "ws://192.168.10.101:8765"
#define FEEDING_PLAN_PATH "/spiffs/feeding_plan.json"
#define VERSION_CHECK_INTERVAL 3600 // 1小时检查一次

static const char *TAG = "WS_CLIENT";
static feeding_plan_t g_plans[MAX_FEEDING_PLANS];
static int g_plan_count = 0;
esp_websocket_client_handle_t g_ws_client = NULL;

// 记录上次定时计划执行的时间（按计划索引）
static time_t g_last_plan_exec[MAX_FEEDING_PLANS] = {0};

// WebSocket重连标志
volatile bool g_ws_need_reconnect = false;

// 版本检查间隔（秒）
static time_t g_last_version_check = 0;
static bool g_has_available_update = false;  // 新增：记录是否有可用更新
static char g_latest_version[32] = {0};      // 新增：记录最新版本号
static char g_download_url[256] = {0};       // 新增：记录下载URL

esp_err_t feeding_plan_save_all_to_spiffs(void) {
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < g_plan_count; ++i) {
        cJSON *obj = cJSON_CreateObject();
        cJSON_AddNumberToObject(obj, "day_of_week", g_plans[i].day_of_week);
        cJSON_AddNumberToObject(obj, "hour", g_plans[i].hour);
        cJSON_AddNumberToObject(obj, "minute", g_plans[i].minute);
        cJSON_AddNumberToObject(obj, "feeding_amount", g_plans[i].feeding_amount);
        cJSON_AddItemToArray(arr, obj);
    }
    char *json_str = cJSON_PrintUnformatted(arr);
    FILE *fp = fopen(FEEDING_PLAN_PATH, "w");
    if (!fp) {
        ESP_LOGE(TAG, "无法写入! %s", FEEDING_PLAN_PATH);
        cJSON_Delete(arr);
        free(json_str);
        return ESP_FAIL;
    }
    fwrite(json_str, 1, strlen(json_str), fp);
    fclose(fp);
    cJSON_Delete(arr);
    free(json_str);
    ESP_LOGI(TAG, "所有喂食计划已保存到 %s", FEEDING_PLAN_PATH);
    return ESP_OK;
}

esp_err_t feeding_plan_load_all_from_spiffs(void) {
    ESP_LOGI(TAG, "=== 开始加载喂食计划文件: %s ===", FEEDING_PLAN_PATH);
    g_plan_count = 0;
    FILE *fp = fopen(FEEDING_PLAN_PATH, "r");
    if (!fp) {
        ESP_LOGW(TAG, "=== 未找到喂食计划文件 %s ===", FEEDING_PLAN_PATH);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "=== 成功打开喂食计划文件 ===");
    
    char buf[1024] = {0};
    size_t len = fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    ESP_LOGI(TAG, "=== 读取到 %zu 字节数据 ===", len);
    
    if (len == 0) {
        ESP_LOGW(TAG, "=== 喂食计划文件为空 ===");
        return ESP_FAIL;
    }

    cJSON *arr = cJSON_Parse(buf);
    if (!arr || !cJSON_IsArray(arr)) {
        ESP_LOGW(TAG, "=== JSON解析失败或不是数组格式 ===");
        if (arr) cJSON_Delete(arr);
        return ESP_FAIL;
    }
    
    int count = cJSON_GetArraySize(arr);
    ESP_LOGI(TAG, "=== JSON数组包含 %d 个元素 ===", count);
    
    for (int i = 0; i < count && i < MAX_FEEDING_PLANS; ++i) {
        cJSON *obj = cJSON_GetArrayItem(arr, i);
        g_plans[i].day_of_week = cJSON_GetObjectItem(obj, "day_of_week")->valueint;
        g_plans[i].hour = cJSON_GetObjectItem(obj, "hour")->valueint;
        g_plans[i].minute = cJSON_GetObjectItem(obj, "minute")->valueint;
        g_plans[i].feeding_amount = (float)cJSON_GetObjectItem(obj, "feeding_amount")->valuedouble;
        ESP_LOGI(TAG, "=== 加载计划[%d]: 星期%d %02d:%02d %.1fg ===", 
                i, g_plans[i].day_of_week, g_plans[i].hour, g_plans[i].minute, g_plans[i].feeding_amount);
    }
    g_plan_count = count < MAX_FEEDING_PLANS ? count : MAX_FEEDING_PLANS;
    cJSON_Delete(arr);
    ESP_LOGI(TAG, "=== 已加载%d条喂食计划 ===", g_plan_count);
    return ESP_OK;
}

int feeding_plan_add(const feeding_plan_t *plan) {
    ESP_LOGI(TAG, "=== 添加喂食计划: 星期%d %02d:%02d %.1fg ===", 
            plan->day_of_week, plan->hour, plan->minute, plan->feeding_amount);
    
    // 查找是否有相同时间的计划
    for (int i = 0; i < g_plan_count; ++i) {
        if (g_plans[i].day_of_week == plan->day_of_week &&
            g_plans[i].hour == plan->hour &&
            g_plans[i].minute == plan->minute) {
            // 克数相加
            g_plans[i].feeding_amount += plan->feeding_amount;
            ESP_LOGI(TAG, "=== 找到相同时间计划，克数相加后: %.1fg ===", g_plans[i].feeding_amount);
            feeding_plan_save_all_to_spiffs();
            return i;
        }
    }
    if (g_plan_count >= MAX_FEEDING_PLANS) {
        ESP_LOGW(TAG, "=== 喂食计划数量已达上限 %d ===", MAX_FEEDING_PLANS);
        return -1;
    }
    g_plans[g_plan_count++] = *plan;
    ESP_LOGI(TAG, "=== 成功添加新计划，当前计划总数: %d ===", g_plan_count);
    feeding_plan_save_all_to_spiffs();
    return g_plan_count - 1;
}

int feeding_plan_delete(int index) {
    if (index < 0 || index >= g_plan_count) return -1;
    for (int i = index; i < g_plan_count - 1; ++i) {
        g_plans[i] = g_plans[i + 1];
    }
    g_plan_count--;
    feeding_plan_save_all_to_spiffs();
    return 0;
}

int feeding_plan_count(void) {
    return g_plan_count;
}

const feeding_plan_t* feeding_plan_get(int index) {
    if (index < 0 || index >= g_plan_count) return NULL;
    return &g_plans[index];
}

// 简化版本的喂食计划检查函数，用于定时器回调
void feeding_plan_check_and_execute_simple(void) {
    time_info_t now;
    esp_err_t time_ret = get_current_time_info(&now);
    if (time_ret != ESP_OK) {
        return; // 静默失败，不打印日志
    }
    
    // 只打印时间，不打印其他详细信息
    ESP_LOGI(TAG, "=== 当前时间: 星期%d %02d:%02d ===", now.weekday, now.hour, now.minute);
    
    // 定时计划 - 简化处理
    for (int i = 0; i < g_plan_count; ++i) {
        if (g_plans[i].day_of_week == now.weekday &&
            g_plans[i].hour == now.hour &&
            g_plans[i].minute == now.minute) {
            // 只在第一次触发时执行，1分钟内不重复
            time_t t = time(NULL);
            if (g_last_plan_exec[i] == 0 || t - g_last_plan_exec[i] > 50) {
                g_last_plan_exec[i] = t;
                ESP_LOGI(TAG, "自动执行喂食计划: 星期%d %02d:%02d %.1fg",
                         g_plans[i].day_of_week, g_plans[i].hour, g_plans[i].minute, g_plans[i].feeding_amount);
                // 调用电机喂食函数
                motor_feed(g_plans[i].feeding_amount);
                // 上报喂食记录
                if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                    const char *device_id = device_manager_get_device_id();
                    static char report_msg[160];
                    snprintf(report_msg, sizeof(report_msg),
                        "{\"type\":\"feeding_record\",\"device_id\":\"%s\",\"day_of_week\":%d,\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f,\"timestamp\":%lld}",
                        device_id,
                        g_plans[i].day_of_week,
                        g_plans[i].hour,
                        g_plans[i].minute,
                        g_plans[i].feeding_amount,
                        (long long)t);
                    esp_websocket_client_send_text(g_ws_client, report_msg, strlen(report_msg), portMAX_DELAY);
                    ESP_LOGI(TAG, "已上报喂食记录");
                }
            }
        }
    }
    
    // 手动喂食 - 简化处理
    int manual_count = get_manual_feedings_count();
    for (int i = 0; i < manual_count;) {
        manual_feeding_t* mf = get_manual_feeding(i);
        if (mf && mf->hour == now.hour && mf->minute == now.minute) {
            ESP_LOGI(TAG, "执行手动喂食: %02d:%02d %.1fg", mf->hour, mf->minute, mf->feeding_amount);
            // 调用电机喂食函数
            motor_feed(mf->feeding_amount);
            // 上报喂食记录
            if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                const char *device_id = device_manager_get_device_id();
                static char report_msg[160];
                snprintf(report_msg, sizeof(report_msg),
                    "{\"type\":\"manual_feeding\",\"device_id\":\"%s\",\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f,\"timestamp\":%lld}",
                    device_id,
                    mf->hour,
                    mf->minute,
                    mf->feeding_amount,
                    (long long)time(NULL));
                esp_websocket_client_send_text(g_ws_client, report_msg, strlen(report_msg), portMAX_DELAY);
                ESP_LOGI(TAG, "已上报手动喂食记录");
            }
            // 删除已执行的手动喂食
            int id = mf->id;
            delete_manual_feeding(id);
            manual_count = get_manual_feedings_count(); // 更新数量
            // 不递增i，因已删除当前项
        } else {
            ++i;
        }
    }
}

void feeding_plan_check_and_execute(void) {
    ESP_LOGI(TAG, "=== 进入喂食计划检查函数 ===");
    time_info_t now;
    esp_err_t time_ret = get_current_time_info(&now);
    if (time_ret != ESP_OK) {
        ESP_LOGW(TAG, "=== 获取当前时间失败，错误码: %s ===", esp_err_to_name(time_ret));
        return;
    }
    ESP_LOGI(TAG, "=== 当前时间: 星期%d %02d:%02d ===", now.weekday, now.hour, now.minute);
    
    // 使用静态缓冲区减少栈使用
    static char report_msg[160];
    
    // 定时计划
    for (int i = 0; i < g_plan_count; ++i) {
        if (g_plans[i].day_of_week == now.weekday &&
            g_plans[i].hour == now.hour &&
            g_plans[i].minute == now.minute) {
            // 只在第一次触发时执行，1分钟内不重复
            time_t t = time(NULL);
            if (g_last_plan_exec[i] == 0 || t - g_last_plan_exec[i] > 50) {
                g_last_plan_exec[i] = t;
                ESP_LOGI(TAG, "自动执行喂食计划: 星期%d %02d:%02d %.1fg",
                         g_plans[i].day_of_week, g_plans[i].hour, g_plans[i].minute, g_plans[i].feeding_amount);
                // 调用电机喂食函数
                motor_feed(g_plans[i].feeding_amount);
                // 上报喂食记录
                if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                    const char *device_id = device_manager_get_device_id();
                    snprintf(report_msg, sizeof(report_msg),
                        "{\"type\":\"feeding_record\",\"device_id\":\"%s\",\"day_of_week\":%d,\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f,\"timestamp\":%lld}",
                        device_id,
                        g_plans[i].day_of_week,
                        g_plans[i].hour,
                        g_plans[i].minute,
                        g_plans[i].feeding_amount,
                        (long long)t);
                    esp_websocket_client_send_text(g_ws_client, report_msg, strlen(report_msg), portMAX_DELAY);
                    ESP_LOGI(TAG, "已上报喂食记录");
                }
            }
        }
    }
    
    // 手动喂食
    int manual_count = get_manual_feedings_count();
    for (int i = 0; i < manual_count;) {
        manual_feeding_t* mf = get_manual_feeding(i);
        if (mf && mf->hour == now.hour && mf->minute == now.minute) {
            ESP_LOGI(TAG, "执行手动喂食: %02d:%02d %.1fg", mf->hour, mf->minute, mf->feeding_amount);
            // 调用电机喂食函数
            motor_feed(mf->feeding_amount);
            // 上报喂食记录
            if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                const char *device_id = device_manager_get_device_id();
                snprintf(report_msg, sizeof(report_msg),
                    "{\"type\":\"manual_feeding\",\"device_id\":\"%s\",\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f,\"timestamp\":%lld}",
                    device_id,
                    mf->hour,
                    mf->minute,
                    mf->feeding_amount,
                    (long long)time(NULL));
                esp_websocket_client_send_text(g_ws_client, report_msg, strlen(report_msg), portMAX_DELAY);
                ESP_LOGI(TAG, "已上报手动喂食记录");
            }
            // 删除已执行的手动喂食
            int id = mf->id;
            delete_manual_feeding(id);
            manual_count = get_manual_feedings_count(); // 更新数量
            // 不递增i，因已删除当前项
        } else {
            ++i;
        }
    }
}

static void parse_and_add_plan(const char *json) {
    cJSON *root = cJSON_Parse(json);
    if (!root) return;
    feeding_plan_t plan = {0};
    int ws_day = 0;
    cJSON *day_item = cJSON_GetObjectItem(root, "day_of_week");
    if (cJSON_IsString(day_item)) {
        ws_day = atoi(day_item->valuestring);
    } else {
        ws_day = day_item->valueint;
    }
    if (ws_day == 7) {
        plan.day_of_week = 0; // 周日
    } else {
        plan.day_of_week = ws_day; // 1-6
    }
    // hour
    cJSON *hour_item = cJSON_GetObjectItem(root, "hour");
    if (cJSON_IsString(hour_item)) {
        plan.hour = atoi(hour_item->valuestring);
    } else {
        plan.hour = hour_item->valueint;
    }
    // minute
    cJSON *minute_item = cJSON_GetObjectItem(root, "minute");
    if (cJSON_IsString(minute_item)) {
        plan.minute = atoi(minute_item->valuestring);
    } else {
        plan.minute = minute_item->valueint;
    }
    // feeding_amount
    cJSON *amount_item = cJSON_GetObjectItem(root, "feeding_amount");
    if (cJSON_IsString(amount_item)) {
        plan.feeding_amount = atof(amount_item->valuestring);
    } else {
        plan.feeding_amount = (float)amount_item->valuedouble;
    }
    cJSON_Delete(root);
    feeding_plan_add(&plan);
}

// OTA任务，支持动态URL
void ota_task(void *pvParameter)
{
    char *url = (char *)pvParameter;
    const char *device_id = device_manager_get_device_id();
    
    // 动态分配URL缓冲区，减少栈使用
    char *url_with_id = malloc(256);
    if (!url_with_id) {
        ESP_LOGE("OTA", "Failed to allocate URL buffer");
        // 简化错误处理，不调用update_ota_status避免NVS操作
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    if (strchr(url, '?')) {
        snprintf(url_with_id, 256, "%s&device_id=%s", url, device_id);
    } else {
        snprintf(url_with_id, 256, "%s?device_id=%s", url, device_id);
    }
    ESP_LOGI("OTA", "Starting OTA, url: %s", url_with_id);
    
    // 简化状态更新，避免复杂的NVS操作
    ESP_LOGI("OTA", "OTA status: DOWNLOADING, progress: 0 percent");
    
    // 动态分配HTTP配置，减少栈使用
    esp_http_client_config_t *http_config = malloc(sizeof(esp_http_client_config_t));
    if (!http_config) {
        ESP_LOGE("OTA", "Failed to allocate HTTP config");
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    memset(http_config, 0, sizeof(esp_http_client_config_t));
    http_config->url = url_with_id;
    http_config->skip_cert_common_name_check = true;
    http_config->disable_auto_redirect = true;
    
    esp_http_client_handle_t client = esp_http_client_init(http_config);
    free(http_config);  // 立即释放配置内存
    
    if (client == NULL) {
        ESP_LOGE("OTA", "Failed to initialize HTTP client");
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length <= 0) {
        ESP_LOGE("OTA", "Invalid content length: %d", content_length);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI("OTA", "Content length: %d", content_length);
    
    // Get the next OTA partition
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE("OTA", "No OTA partition available");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI("OTA", "Writing to partition subtype %d at offset 0x%" PRIx32,
             update_partition->subtype, update_partition->address);
    
    esp_ota_handle_t update_handle = 0;
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "esp_ota_begin failed: %s", esp_err_to_name(err));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    char *buffer = malloc(256);  // 进一步减少缓冲区大小
    if (buffer == NULL) {
        ESP_LOGE("OTA", "Failed to allocate buffer");
        esp_ota_abort(update_handle);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        free(url_with_id);
        free(url);
        vTaskDelete(NULL);
        return;
    }
    
    int total_read = 0;
    int written = 0;
    
    while (total_read < content_length) {
        int read_len = esp_http_client_read(client, buffer, 256);  // 减少读取大小
        if (read_len <= 0) {
            ESP_LOGE("OTA", "Error reading data");
            break;
        }
        
        err = esp_ota_write(update_handle, buffer, read_len);
        if (err != ESP_OK) {
            ESP_LOGE("OTA", "esp_ota_write failed: %s", esp_err_to_name(err));
            break;
        }
        
        total_read += read_len;
        written += read_len;
        
        // 简化进度更新，只打印日志
        int progress = (total_read * 100) / content_length;
        ESP_LOGI("OTA", "Progress: %d%%, Written %d bytes, total: %d/%d", 
                progress, read_len, total_read, content_length);
    }
    
    free(buffer);
    
    if (total_read == content_length) {
        ESP_LOGI("OTA", "Download completed, finalizing...");
        
        err = esp_ota_end(update_handle);
        if (err == ESP_OK) {
            err = esp_ota_set_boot_partition(update_partition);
            if (err == ESP_OK) {
                ESP_LOGI("OTA", "OTA upgrade successful, restarting...");
                // 清除更新状态
                websocket_clear_update_status();
                vTaskDelay(pdMS_TO_TICKS(1000)); // 等待状态更新完成
                esp_restart();
            } else {
                ESP_LOGE("OTA", "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
            }
        } else {
            ESP_LOGE("OTA", "esp_ota_end failed: %s", esp_err_to_name(err));
        }
    } else {
        esp_ota_abort(update_handle);
        ESP_LOGE("OTA", "OTA upgrade failed: incomplete download (%d/%d)", total_read, content_length);
    }
    
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    free(url_with_id);
    free(url);
    vTaskDelete(NULL);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    esp_websocket_client_handle_t client = (esp_websocket_client_handle_t)handler_args;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            {
                const char *device_id = device_manager_get_device_id();
                char reg_msg[256];
                
                // 获取版本信息
                const firmware_version_t *fw_version = get_firmware_version();
                const protocol_version_t *proto_version = get_protocol_version();
                const hardware_version_t *hw_version = get_hardware_version();
                
                char version_str[64];
                get_version_string(fw_version, version_str, sizeof(version_str));
                
                snprintf(reg_msg, sizeof(reg_msg), 
                    "{\"device_id\":\"%s\",\"firmware_version\":\"%s\",\"protocol_version\":\"%d.%d\",\"hardware_version\":\"%d.%d\"}", 
                    device_id, version_str, proto_version->major, proto_version->minor, 
                    hw_version->major, hw_version->minor);
                
                esp_websocket_client_send_text(client, reg_msg, strlen(reg_msg), portMAX_DELAY);
                ESP_LOGI(TAG, "Sent register with version info: %s", reg_msg);
                
                // 连接成功后再次尝试加载喂食计划
                ESP_LOGI(TAG, "=== WebSocket连接成功，重新加载喂食计划 ===");
                feeding_plan_load_all_from_spiffs();
            }
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
            // 只设置重连标志，由专门任务处理重连
            g_ws_need_reconnect = true;
            break;
        case WEBSOCKET_EVENT_DATA:
            if (data->data_len > 0) {
                ESP_LOGI(TAG, "Received data: %.*s", data->data_len, (char *)data->data_ptr);
                char msg[1024] = {0};
                int len = data->data_len < sizeof(msg) - 1 ? data->data_len : sizeof(msg) - 1;
                memcpy(msg, data->data_ptr, len);
                ESP_LOGI(TAG, "WebSocket收到原始数据: %s", msg);
                cJSON *root = cJSON_Parse(msg);
                if (root) {
                    cJSON *type_item = cJSON_GetObjectItem(root, "type");
                    if (type_item && cJSON_IsString(type_item)) {
                        if (strcmp(type_item->valuestring, "register_result") == 0) {
                            // 处理注册结果，保存device_id和password
                            cJSON *id_item = cJSON_GetObjectItem(root, "device_id");
                            cJSON *pwd_item = cJSON_GetObjectItem(root, "password");
                            if (id_item && pwd_item && cJSON_IsString(id_item) && cJSON_IsString(pwd_item)) {
                                ESP_LOGI(TAG, "收到注册结果: device_id=%s, password=%s", id_item->valuestring, pwd_item->valuestring);
                                // 保存到全局变量和SPIFFS
                                extern void device_manager_set_device_info(const char *id, const char *pwd); // 若有此函数
                                device_manager_set_device_info(id_item->valuestring, pwd_item->valuestring);
                            }
                        } else if (strcmp(type_item->valuestring, "version_check") == 0) {
                            // 处理版本检查请求
                            ESP_LOGI(TAG, "收到版本检查请求");
                            const char *device_id = device_manager_get_device_id();
                            const firmware_version_t *fw_version = get_firmware_version();
                            const protocol_version_t *proto_version = get_protocol_version();
                            const hardware_version_t *hw_version = get_hardware_version();
                            
                            char version_str[64];
                            get_version_string(fw_version, version_str, sizeof(version_str));
                            
                            char response[512];
                            snprintf(response, sizeof(response),
                                "{\"type\":\"version_check_result\",\"device_id\":\"%s\",\"firmware_version\":\"%s\",\"protocol_version\":\"%d.%d\",\"hardware_version\":\"%d.%d\"}",
                                device_id, version_str, proto_version->major, proto_version->minor,
                                hw_version->major, hw_version->minor);
                            
                            esp_websocket_client_send_text(client, response, strlen(response), portMAX_DELAY);
                            ESP_LOGI(TAG, "已回复版本检查: %s", response);
                            
                        } else if (strcmp(type_item->valuestring, "version_check_result") == 0) {
                            // 处理版本检查响应
                            ESP_LOGI(TAG, "收到版本检查结果: %s", msg);
                            
                            cJSON *has_update = cJSON_GetObjectItem(root, "has_update");
                            cJSON *latest_version = cJSON_GetObjectItem(root, "latest_version");
                            cJSON *download_url = cJSON_GetObjectItem(root, "download_url");
                            cJSON *force_update = cJSON_GetObjectItem(root, "force_update");
                            
                            ESP_LOGI(TAG, "版本检查结果解析: has_update=%s, latest_version=%s, download_url=%s, force_update=%s",
                                    has_update ? (cJSON_IsTrue(has_update) ? "true" : "false") : "null",
                                    latest_version ? latest_version->valuestring : "null",
                                    download_url ? download_url->valuestring : "null",
                                    force_update ? (cJSON_IsTrue(force_update) ? "true" : "false") : "null");
                            
                            if (has_update && cJSON_IsTrue(has_update) && latest_version && download_url) {
                                ESP_LOGI(TAG, "发现新版本: %s", latest_version->valuestring);
                                
                                // 检查是否需要强制更新
                                bool force = (force_update && cJSON_IsTrue(force_update));
                                ESP_LOGI(TAG, "强制更新标志: %s", force ? "true" : "false");
                                
                                // 验证版本兼容性
                                const firmware_version_t *current = get_firmware_version();
                                ESP_LOGI(TAG, "当前版本: v%d.%d.%d", current->major, current->minor, current->patch);
                                
                                firmware_version_t target = {0};
                                
                                // 解析目标版本号（支持带v和不带v的格式）
                                int parsed = 0;
                                if (sscanf(latest_version->valuestring, "v%hhu.%hhu.%hhu", 
                                         &target.major, &target.minor, &target.patch) == 3) {
                                    parsed = 1;
                                } else if (sscanf(latest_version->valuestring, "%hhu.%hhu.%hhu", 
                                                &target.major, &target.minor, &target.patch) == 3) {
                                    parsed = 1;
                                }
                                
                                if (parsed) {
                                    ESP_LOGI(TAG, "目标版本: v%d.%d.%d", target.major, target.minor, target.patch);
                                    
                                    bool is_compatible = check_firmware_compatibility(current, &target);
                                    ESP_LOGI(TAG, "版本兼容性检查结果: %s", is_compatible ? "true" : "false");
                                    
                                    // 修改：不再自动升级，只记录版本信息
                                    if (is_compatible || force) {
                                        ESP_LOGI(TAG, "发现可用更新，等待管理员手动升级指令");
                                        // 保存版本信息，供后续使用
                                        g_has_available_update = true;
                                        strncpy(g_latest_version, latest_version->valuestring, sizeof(g_latest_version) - 1);
                                        strncpy(g_download_url, download_url->valuestring, sizeof(g_download_url) - 1);
                                        ESP_LOGI(TAG, "已保存更新信息: 版本=%s, URL=%s", g_latest_version, g_download_url);
                                    } else {
                                        ESP_LOGW(TAG, "版本不兼容，无法升级");
                                        g_has_available_update = false;
                                    }
                                } else {
                                    ESP_LOGE(TAG, "无法解析版本号: %s", latest_version->valuestring);
                                }
                            } else {
                                ESP_LOGI(TAG, "当前已是最新版本或缺少必要参数");
                            }
                        } else if (strcmp(type_item->valuestring, "rollback_request") == 0) {
                            // 处理版本回滚请求
                            cJSON *target_version = cJSON_GetObjectItem(root, "target_version");
                            cJSON *reason = cJSON_GetObjectItem(root, "reason");
                            
                            if (target_version && cJSON_IsString(target_version)) {
                                ESP_LOGI(TAG, "收到版本回滚请求: %s, 原因: %s", 
                                        target_version->valuestring, 
                                        reason ? reason->valuestring : "unknown");
                                
                                // 执行版本回滚
                                esp_err_t ret = rollback_version(target_version->valuestring);
                                
                                // 回复回滚结果
                                const char *device_id = device_manager_get_device_id();
                                char response[256];
                                snprintf(response, sizeof(response),
                                    "{\"type\":\"rollback_result\",\"device_id\":\"%s\",\"target_version\":\"%s\",\"success\":%s}",
                                    device_id, target_version->valuestring, 
                                    (ret == ESP_OK) ? "true" : "false");
                                
                                esp_websocket_client_send_text(client, response, strlen(response), portMAX_DELAY);
                                ESP_LOGI(TAG, "已回复回滚结果: %s", response);
                            }
                            
                        } else if (strcmp(type_item->valuestring, "delete_feeding_plan") == 0) {
                            cJSON *dw = cJSON_GetObjectItem(root, "day_of_week");
                            cJSON *hour = cJSON_GetObjectItem(root, "hour");
                            cJSON *minute = cJSON_GetObjectItem(root, "minute");
                            cJSON *amount = cJSON_GetObjectItem(root, "feeding_amount");
                            int deleted = 0;
                            if (dw && hour && minute && amount) {
                                for (int i = 0; i < g_plan_count; ++i) {
                                    if (g_plans[i].day_of_week == dw->valueint &&
                                        g_plans[i].hour == hour->valueint &&
                                        g_plans[i].minute == minute->valueint &&
                                        fabs(g_plans[i].feeding_amount - (float)amount->valuedouble) < 0.01f) {
                                        ESP_LOGI(TAG, "收到删除喂食计划指令: 匹配到本地计划索引%d", i);
                                        feeding_plan_delete(i);
                                        deleted = 1;
                                        break;
                                    }
                                }
                            }
                            if (!deleted) {
                                ESP_LOGW(TAG, "未能根据时间信息删除喂食计划");
                            }
                            // 回复删除计划确认
                            const char *device_id = device_manager_get_device_id();
                            int day_of_week = dw ? (cJSON_IsString(dw) ? atoi(dw->valuestring) : dw->valueint) : 0;
                            int hour_val = hour ? (cJSON_IsString(hour) ? atoi(hour->valuestring) : hour->valueint) : 0;
                            int minute_val = minute ? (cJSON_IsString(minute) ? atoi(minute->valuestring) : minute->valueint) : 0;
                            float feeding_amount = amount ? (cJSON_IsString(amount) ? atof(amount->valuestring) : (float)amount->valuedouble) : 0.0f;
                            char confirm_msg[160];
                            snprintf(confirm_msg, sizeof(confirm_msg),
                                "{\"type\":\"confirm_delete_feeding_plan\",\"device_id\":\"%s\",\"day_of_week\":%d,\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f}",
                                device_id, day_of_week, hour_val, minute_val, feeding_amount);
                            esp_websocket_client_send_text(client, confirm_msg, strlen(confirm_msg), portMAX_DELAY);
                            ESP_LOGI(TAG, "已回复删除计划确认: %s", confirm_msg);
                        } else if (strcmp(type_item->valuestring, "delete_manual_feeding") == 0) {
                            cJSON *hour = cJSON_GetObjectItem(root, "hour");
                            cJSON *minute = cJSON_GetObjectItem(root, "minute");
                            cJSON *amount = cJSON_GetObjectItem(root, "feeding_amount");
                            int deleted = 0;
                            int manual_count = get_manual_feedings_count();
                            if (hour && minute && amount) {
                                for (int i = 0; i < manual_count; ++i) {
                                    manual_feeding_t* mf = get_manual_feeding(i);
                                    if (mf && mf->hour == hour->valueint &&
                                        mf->minute == minute->valueint &&
                                        fabs(mf->feeding_amount - (float)amount->valuedouble) < 0.01f) {
                                        ESP_LOGI(TAG, "收到删除手动喂食指令: 匹配到本地手动喂食索引%d", i);
                                        delete_manual_feeding(mf->id);
                                        deleted = 1;
                                        break;
                                    }
                                }
                            }
                            if (!deleted) {
                                ESP_LOGW(TAG, "未能根据时间信息删除手动喂食");
                            }
                            // 回复删除手动喂食确认
                            const char *device_id = device_manager_get_device_id();
                            int hour_val = hour ? (cJSON_IsString(hour) ? atoi(hour->valuestring) : hour->valueint) : 0;
                            int minute_val = minute ? (cJSON_IsString(minute) ? atoi(minute->valuestring) : minute->valueint) : 0;
                            float feeding_amount = amount ? (cJSON_IsString(amount) ? atof(amount->valuestring) : (float)amount->valuedouble) : 0.0f;
                            char confirm_msg[160];
                            snprintf(confirm_msg, sizeof(confirm_msg),
                                "{\"type\":\"confirm_delete_manual_feeding\",\"device_id\":\"%s\",\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f}",
                                device_id, hour_val, minute_val, feeding_amount);
                            esp_websocket_client_send_text(client, confirm_msg, strlen(confirm_msg), portMAX_DELAY);
                            ESP_LOGI(TAG, "已回复删除手动喂食确认: %s", confirm_msg);
                        } else if (strcmp(type_item->valuestring, "feeding_plan") == 0 || cJSON_GetObjectItem(root, "day_of_week")) {
                            ESP_LOGI(TAG, "收到喂食计划: %s", msg);
                            parse_and_add_plan(msg);
                            // 回复确认
                            const char *device_id = device_manager_get_device_id();
                            cJSON *dw_item = cJSON_GetObjectItem(root, "day_of_week");
                            cJSON *hour_item = cJSON_GetObjectItem(root, "hour");
                            cJSON *minute_item = cJSON_GetObjectItem(root, "minute");
                            cJSON *amount_item = cJSON_GetObjectItem(root, "feeding_amount");
                            int day_of_week = dw_item ? (cJSON_IsString(dw_item) ? atoi(dw_item->valuestring) : dw_item->valueint) : 0;
                            int hour = hour_item ? (cJSON_IsString(hour_item) ? atoi(hour_item->valuestring) : hour_item->valueint) : 0;
                            int minute = minute_item ? (cJSON_IsString(minute_item) ? atoi(minute_item->valuestring) : minute_item->valueint) : 0;
                            float feeding_amount = amount_item ? (cJSON_IsString(amount_item) ? atof(amount_item->valuestring) : (float)amount_item->valuedouble) : 0.0f;
                            char confirm_msg[160];
                            snprintf(confirm_msg, sizeof(confirm_msg),
                                "{\"type\":\"confirm_feeding_plan\",\"device_id\":\"%s\",\"day_of_week\":%d,\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f}",
                                device_id, day_of_week, hour, minute, feeding_amount);
                            esp_websocket_client_send_text(client, confirm_msg, strlen(confirm_msg), portMAX_DELAY);
                            ESP_LOGI(TAG, "已回复计划确认: %s", confirm_msg);
                        } else if (strcmp(type_item->valuestring, "manual_feeding") == 0 || (cJSON_GetObjectItem(root, "hour") && cJSON_GetObjectItem(root, "minute") && cJSON_GetObjectItem(root, "feeding_amount"))) {
                            ESP_LOGI(TAG, "收到手动喂食: %s", msg);
                            int hour = 0, minute = 0;
                            float amount = 0;
                            cJSON *hour_item = cJSON_GetObjectItem(root, "hour");
                            cJSON *minute_item = cJSON_GetObjectItem(root, "minute");
                            cJSON *amount_item = cJSON_GetObjectItem(root, "feeding_amount");
                            if (cJSON_IsString(hour_item)) hour = atoi(hour_item->valuestring);
                            else hour = hour_item->valueint;
                            if (cJSON_IsString(minute_item)) minute = atoi(minute_item->valuestring);
                            else minute = minute_item->valueint;
                            if (cJSON_IsString(amount_item)) amount = atof(amount_item->valuestring);
                            else amount = (float)amount_item->valuedouble;
                            // 先查找是否有相同时间的手动喂食，有则克数相加
                            int manual_count = get_manual_feedings_count();
                            int found = 0;
                            for (int i = 0; i < manual_count; ++i) {
                                manual_feeding_t* mf = get_manual_feeding(i);
                                if (mf && mf->hour == hour && mf->minute == minute) {
                                    mf->feeding_amount += amount;
                                    found = 1;
                                    break;
                                }
                            }
                            if (!found) {
                                add_manual_feeding(hour, minute, amount);
                            }
                            // 回复确认
                            const char *device_id = device_manager_get_device_id();
                            long long timestamp = (long long)time(NULL);
                            char confirm_msg[200];
                            snprintf(confirm_msg, sizeof(confirm_msg),
                                "{\"type\":\"confirm_manual_feeding\",\"device_id\":\"%s\",\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f,\"timestamp\":%lld}",
                                device_id, hour, minute, amount, timestamp);
                            esp_websocket_client_send_text(client, confirm_msg, strlen(confirm_msg), portMAX_DELAY);
                            ESP_LOGI(TAG, "已回复手动喂食确认: %s", confirm_msg);
                        } else if (strcmp(type_item->valuestring, "sync_request") == 0) {
                            ESP_LOGI(TAG, "收到sync_request消息，准备上报sync_result");
                            // 组装sync_result
                            const char *device_id = device_manager_get_device_id();
                            float grain_weight = 0.0f;
                            extern float HX711_get_units(char times); // 确保有此函数
                            grain_weight = HX711_get_units(10);
                            cJSON *result = cJSON_CreateObject();
                            cJSON_AddStringToObject(result, "type", "sync_result");
                            cJSON_AddStringToObject(result, "device_id", device_id);
                            cJSON_AddNumberToObject(result, "grain_weight", grain_weight);
                            // feeding_plans
                            cJSON *plans_arr = cJSON_CreateArray();
                            int plan_count = feeding_plan_count();
                            for (int i = 0; i < plan_count; ++i) {
                                const feeding_plan_t *plan = feeding_plan_get(i);
                                if (plan) {
                                    cJSON *plan_obj = cJSON_CreateObject();
                                    cJSON_AddNumberToObject(plan_obj, "day_of_week", plan->day_of_week);
                                    cJSON_AddNumberToObject(plan_obj, "hour", plan->hour);
                                    cJSON_AddNumberToObject(plan_obj, "minute", plan->minute);
                                    cJSON_AddNumberToObject(plan_obj, "feeding_amount", plan->feeding_amount);
                                    cJSON_AddItemToArray(plans_arr, plan_obj);
                                }
                            }
                            cJSON_AddItemToObject(result, "feeding_plans", plans_arr);
                            // manual_feedings
                            cJSON *manual_arr = cJSON_CreateArray();
                            int manual_count = get_manual_feedings_count();
                            for (int i = 0; i < manual_count; ++i) {
                                manual_feeding_t *mf = get_manual_feeding(i);
                                if (mf) {
                                    cJSON *mf_obj = cJSON_CreateObject();
                                    cJSON_AddNumberToObject(mf_obj, "hour", mf->hour);
                                    cJSON_AddNumberToObject(mf_obj, "minute", mf->minute);
                                    cJSON_AddNumberToObject(mf_obj, "feeding_amount", mf->feeding_amount);
                                    cJSON_AddBoolToObject(mf_obj, "is_confirmed", true); // 默认已确认
                                    cJSON_AddBoolToObject(mf_obj, "is_executed", mf->executed);
                                    if (mf->executed) {
                                        // 这里假设有executed_at字段，若无可置为0或null
                                        cJSON_AddNumberToObject(mf_obj, "executed_at", (double)time(NULL));
                                    } else {
                                        cJSON_AddNullToObject(mf_obj, "executed_at");
                                    }
                                    cJSON_AddItemToArray(manual_arr, mf_obj);
                                }
                            }
                            cJSON_AddItemToObject(result, "manual_feedings", manual_arr);
                            char *json_str = cJSON_PrintUnformatted(result);
                            esp_websocket_client_send_text(client, json_str, strlen(json_str), portMAX_DELAY);
                            ESP_LOGI(TAG, "已回复sync_result: %s", json_str);
                            cJSON_Delete(result);
                            free(json_str);
                        } else if (strcmp(type_item->valuestring, "ota_update") == 0) {
                            cJSON *url_item = cJSON_GetObjectItem(root, "url");
                            cJSON *version_item = cJSON_GetObjectItem(root, "version");
                            
                            if (url_item && cJSON_IsString(url_item)) {
                                // 更新OTA状态为下载中
                                ESP_LOGI(TAG, "OTA status: DOWNLOADING, progress: 0 percent");
                                
                                // 简化OTA信息更新，避免NVS操作
                                ESP_LOGI(TAG, "OTA信息: target_version=%s, download_url=%s", 
                                        version_item ? version_item->valuestring : "unknown", 
                                        url_item->valuestring);
                                
                                // 启动OTA任务
                                char *ota_url = strdup(url_item->valuestring);
                                if (ota_url) {
                                    ESP_LOGI(TAG, "创建OTA任务，URL: %s", ota_url);
                                    
                                    // 检查可用内存
                                    size_t free_heap = esp_get_free_heap_size();
                                    size_t min_free_heap = esp_get_minimum_free_heap_size();
                                    ESP_LOGI(TAG, "当前可用堆内存: %zu bytes", free_heap);
                                    ESP_LOGI(TAG, "最小可用堆内存: %zu bytes", min_free_heap);
                                    
                                    // 尝试清理内存
                                    if (free_heap < 1024 * 1024) {  // 如果可用内存小于1MB
                                        ESP_LOGW(TAG, "可用内存不足，尝试清理");
                                        // 强制垃圾回收（如果有的话）
                                        // 这里可以添加一些内存清理逻辑
                                    }
                                    
                                    // 在OTA升级前暂停一些非关键任务以释放内存
                                    ESP_LOGI(TAG, "OTA升级前暂停非关键任务以释放内存");
                                    // 这里可以添加暂停其他任务的逻辑
                                    
                                    // 减少任务栈大小，从最小开始尝试
                                    BaseType_t task_ret = xTaskCreate(ota_task, "ota_task", 3072, ota_url, 5, NULL);
                                    ESP_LOGI(TAG, "xTaskCreate返回: %s", (task_ret == pdPASS) ? "pdPASS" : "pdFAIL");
                                    if (task_ret != pdPASS) {
                                        ESP_LOGE(TAG, "OTA任务创建失败，尝试更小的栈大小");
                                        // 尝试更小的栈大小
                                        task_ret = xTaskCreate(ota_task, "ota_task", 2048, ota_url, 5, NULL);
                                        ESP_LOGI(TAG, "第二次xTaskCreate返回: %s", (task_ret == pdPASS) ? "pdPASS" : "pdFAIL");
                                        if (task_ret != pdPASS) {
                                            ESP_LOGE(TAG, "OTA任务创建最终失败");
                                            free(ota_url);
                                            update_ota_status(OTA_STATUS_FAILED, 0, 3, "Task creation failed");
                                        } else {
                                            ESP_LOGI(TAG, "OTA任务创建成功（使用2048栈）");
                                        }
                                    } else {
                                        ESP_LOGI(TAG, "OTA任务创建成功，开始下载固件");
                                    }
                                } else {
                                    ESP_LOGE(TAG, "内存分配失败，无法启动OTA任务");
                                    esp_err_t ret = update_ota_status(OTA_STATUS_FAILED, 0, 3, "Memory allocation failed");
                                    ESP_LOGI(TAG, "update_ota_status返回: %s", esp_err_to_name(ret));
                                }
                            } else {
                                ESP_LOGE(TAG, "OTA升级指令缺少URL参数");
                                esp_err_t ret = update_ota_status(OTA_STATUS_FAILED, 0, 4, "Missing URL parameter");
                                ESP_LOGI(TAG, "update_ota_status返回: %s", esp_err_to_name(ret));
                            }
                        } else {
                            ESP_LOGI(TAG, "收到其它消息: %s", msg);
                        }
                    } else if (cJSON_GetObjectItem(root, "status")) {
                        ESP_LOGI(TAG, "注册响应: %s", msg);
                    } else if (cJSON_GetObjectItem(root, "day_of_week")) {
                        ESP_LOGI(TAG, "收到喂食计划: %s", msg);
                        parse_and_add_plan(msg);
                        // 回复确认
                        const char *device_id = device_manager_get_device_id();
                        cJSON *dw_item = cJSON_GetObjectItem(root, "day_of_week");
                        cJSON *hour_item = cJSON_GetObjectItem(root, "hour");
                        cJSON *minute_item = cJSON_GetObjectItem(root, "minute");
                        cJSON *amount_item = cJSON_GetObjectItem(root, "feeding_amount");
                        int day_of_week = dw_item ? (cJSON_IsString(dw_item) ? atoi(dw_item->valuestring) : dw_item->valueint) : 0;
                        int hour = hour_item ? (cJSON_IsString(hour_item) ? atoi(hour_item->valuestring) : hour_item->valueint) : 0;
                        int minute = minute_item ? (cJSON_IsString(minute_item) ? atoi(minute_item->valuestring) : minute_item->valueint) : 0;
                        float feeding_amount = amount_item ? (cJSON_IsString(amount_item) ? atof(amount_item->valuestring) : (float)amount_item->valuedouble) : 0.0f;
                        char confirm_msg[160];
                        snprintf(confirm_msg, sizeof(confirm_msg),
                            "{\"type\":\"confirm_feeding_plan\",\"device_id\":\"%s\",\"day_of_week\":%d,\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f}",
                            device_id, day_of_week, hour, minute, feeding_amount);
                        esp_websocket_client_send_text(client, confirm_msg, strlen(confirm_msg), portMAX_DELAY);
                        ESP_LOGI(TAG, "已回复计划确认: %s", confirm_msg);
                    } else if (cJSON_GetObjectItem(root, "hour") && cJSON_GetObjectItem(root, "minute") && cJSON_GetObjectItem(root, "feeding_amount")) {
                        ESP_LOGI(TAG, "收到手动喂食: %s", msg);
                        int hour = 0, minute = 0;
                        float amount = 0;
                        cJSON *hour_item = cJSON_GetObjectItem(root, "hour");
                        cJSON *minute_item = cJSON_GetObjectItem(root, "minute");
                        cJSON *amount_item = cJSON_GetObjectItem(root, "feeding_amount");
                        if (cJSON_IsString(hour_item)) hour = atoi(hour_item->valuestring);
                        else hour = hour_item->valueint;
                        if (cJSON_IsString(minute_item)) minute = atoi(minute_item->valuestring);
                        else minute = minute_item->valueint;
                        if (cJSON_IsString(amount_item)) amount = atof(amount_item->valuestring);
                        else amount = (float)amount_item->valuedouble;
                        // 先查找是否有相同时间的手动喂食，有则克数相加
                        int manual_count = get_manual_feedings_count();
                        int found = 0;
                        for (int i = 0; i < manual_count; ++i) {
                            manual_feeding_t* mf = get_manual_feeding(i);
                            if (mf && mf->hour == hour && mf->minute == minute) {
                                mf->feeding_amount += amount;
                                found = 1;
                                break;
                            }
                        }
                        if (!found) {
                            add_manual_feeding(hour, minute, amount);
                        }
                        // 回复确认
                        const char *device_id = device_manager_get_device_id();
                        long long timestamp = (long long)time(NULL);
                        char confirm_msg[200];
                        snprintf(confirm_msg, sizeof(confirm_msg),
                            "{\"type\":\"confirm_manual_feeding\",\"device_id\":\"%s\",\"hour\":%d,\"minute\":%d,\"feeding_amount\":%.1f,\"timestamp\":%lld}",
                            device_id, hour, minute, amount, timestamp);
                        esp_websocket_client_send_text(client, confirm_msg, strlen(confirm_msg), portMAX_DELAY);
                        ESP_LOGI(TAG, "已回复手动喂食确认: %s", confirm_msg);
                    } else {
                        ESP_LOGI(TAG, "收到其它消息: %s", msg);
                    }
                    cJSON_Delete(root);
                }
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI(TAG, "WebSocket error");
            break;
    }
}

// WebSocket重连任务
void websocket_reconnect_task(void *arg) {
    while (1) {
        if (g_ws_need_reconnect) {
            ESP_LOGI(TAG, "检测到WebSocket断开，2秒后重连...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_websocket_client_stop(g_ws_client);
            esp_websocket_client_start(g_ws_client);
            g_ws_need_reconnect = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 版本检查任务
void version_check_task(void *arg) {
    while (1) {
        time_t now = time(NULL);
        
        // 检查是否需要版本检查
        if (now - g_last_version_check >= VERSION_CHECK_INTERVAL) {
            if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                ESP_LOGI(TAG, "执行定期版本检查");
                
                const char *device_id = device_manager_get_device_id();
                const firmware_version_t *fw_version = get_firmware_version();
                const protocol_version_t *proto_version = get_protocol_version();
                const hardware_version_t *hw_version = get_hardware_version();
                
                char version_str[64];
                get_version_string(fw_version, version_str, sizeof(version_str));
                
                char check_msg[512];
                snprintf(check_msg, sizeof(check_msg),
                    "{\"type\":\"version_check\",\"device_id\":\"%s\",\"firmware_version\":\"%s\",\"protocol_version\":\"%d.%d\",\"hardware_version\":\"%d.%d\"}",
                    device_id, version_str, proto_version->major, proto_version->minor,
                    hw_version->major, hw_version->minor);
                
                esp_websocket_client_send_text(g_ws_client, check_msg, strlen(check_msg), portMAX_DELAY);
                ESP_LOGI(TAG, "已发送版本检查请求: %s", check_msg);
                
                g_last_version_check = now;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(60000)); // 每分钟检查一次
    }
}

void websocket_client_start(void) {
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "=== 开始启动WebSocket客户端 ===");
    ESP_LOGI(TAG, "==========================================");
    feeding_plan_load_all_from_spiffs(); // 启动时加载所有计划

    const esp_websocket_client_config_t ws_cfg = {
        .uri = WS_SERVER_URI,
        .task_stack = 8192, // 增大WebSocket任务栈
    };
    g_ws_client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(g_ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)g_ws_client);
    esp_websocket_client_start(g_ws_client);
    
    // 启动WebSocket重连任务
    xTaskCreatePinnedToCore(websocket_reconnect_task, "ws_reconnect", 4096, NULL, 5, NULL, 1);
    
    // 启动版本检查任务
    xTaskCreatePinnedToCore(version_check_task, "version_check", 4096, NULL, 5, NULL, 1);
    ESP_LOGI(TAG, "=== WebSocket客户端启动完成 ===");
}

// 新增：获取是否有可用更新的状态
bool websocket_has_available_update(void) {
    return g_has_available_update;
}

// 新增：获取最新版本号
const char* websocket_get_latest_version(void) {
    return g_latest_version;
}

// 新增：获取下载URL
const char* websocket_get_download_url(void) {
    return g_download_url;
}

// 新增：清除更新状态（升级完成后调用）
void websocket_clear_update_status(void) {
    g_has_available_update = false;
    memset(g_latest_version, 0, sizeof(g_latest_version));
    memset(g_download_url, 0, sizeof(g_download_url));
} 