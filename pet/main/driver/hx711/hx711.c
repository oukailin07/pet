#include "HX711.h"
#include "esp_log.h"
#include <rom/ets_sys.h>
#include "http_client.h"
#include <math.h>
#include "device_manager.h"
#include "esp_websocket_client.h"
#define HIGH 1
#define LOW 0
#define CLOCK_DELAY_US 20

#define DEBUGTAG "HX711"

static gpio_num_t GPIO_PD_SCK = GPIO_NUM_5;	// Power Down and Serial Clock Input Pin
static gpio_num_t GPIO_DOUT = GPIO_NUM_4;		// Serial Data Output Pin
static HX711_GAIN GAIN = eGAIN_128;		// amplification factor
static unsigned long OFFSET = 0;	// used for tare weight
static float SCALE = 1;	// used to return weight in grams, kg, ounces, whatever
static const char* TAG = "HX711_TEST";



void HX711_init(gpio_num_t dout, gpio_num_t pd_sck, HX711_GAIN gain )
{
	GPIO_PD_SCK = pd_sck;
	GPIO_DOUT = dout;

	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_PD_SCK);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL<<GPIO_DOUT);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	HX711_set_gain(gain);
}

bool HX711_is_ready()
{
	return gpio_get_level(GPIO_DOUT);
}

void HX711_set_gain(HX711_GAIN gain)
{
	GAIN = gain;
	gpio_set_level(GPIO_PD_SCK, LOW);
	HX711_read();
}

uint8_t HX711_shiftIn()
{
	uint8_t value = 0;

    for(int i = 0; i < 8; ++i) 
    {
        gpio_set_level(GPIO_PD_SCK, HIGH);
        ets_delay_us(CLOCK_DELAY_US);
        value |= gpio_get_level(GPIO_DOUT) << (7 - i); //get level returns 
        gpio_set_level(GPIO_PD_SCK, LOW);
        ets_delay_us(CLOCK_DELAY_US);
    }

    return value;
}

unsigned long HX711_read()
{
	gpio_set_level(GPIO_PD_SCK, LOW);
	
	// wait for the chip to become ready with timeout
	int timeout_count = 0;
	const int MAX_TIMEOUT = 1000; // 最大等待1000次，约10秒
	
	while (HX711_is_ready() && timeout_count < MAX_TIMEOUT) 
	{
		vTaskDelay(10 / portTICK_PERIOD_MS);
		timeout_count++;
	}
	
	if (timeout_count >= MAX_TIMEOUT) {
		ESP_LOGE(TAG, "HX711读取超时，传感器可能未连接或损坏");
		return 0;
	}

	unsigned long value = 0;

	//--- Enter critical section ----
	portDISABLE_INTERRUPTS();

	for(int i=0; i < 24 ; i++)
	{   
		gpio_set_level(GPIO_PD_SCK, HIGH);
        ets_delay_us(CLOCK_DELAY_US);
        value = value << 1;
        gpio_set_level(GPIO_PD_SCK, LOW);
        ets_delay_us(CLOCK_DELAY_US);

        if(gpio_get_level(GPIO_DOUT))
        	value++;
	}

	// set the channel and the gain factor for the next reading using the clock pin
	for (unsigned int i = 0; i < GAIN; i++) 
	{	
		gpio_set_level(GPIO_PD_SCK, HIGH);
		ets_delay_us(CLOCK_DELAY_US);
		gpio_set_level(GPIO_PD_SCK, LOW);
		ets_delay_us(CLOCK_DELAY_US);
	}	
	portENABLE_INTERRUPTS();
	//--- Exit critical section ----

	value = value^0x800000;
	
	// 验证读取值的合理性
	if (value == 0 || value > 0x7FFFFF) {
		ESP_LOGW(TAG, "HX711读取值异常: %ld", value);
	}

	return (value);
}



unsigned long  HX711_read_average(char times) 
{
	//ESP_LOGI(DEBUGTAG, "===================== READ AVERAGE START ====================");
	unsigned long sum = 0;
	for (char i = 0; i < times; i++) 
	{
		sum += HX711_read();
	}
	//ESP_LOGI(DEBUGTAG, "===================== READ AVERAGE END : %ld ====================",(sum / times));
	return sum / times;
}

unsigned long HX711_get_value(char times) 
{
	unsigned long avg = HX711_read_average(times);
	if(avg > OFFSET)
		return avg - OFFSET;
	else
		return 0;
}

float HX711_get_units(char times) 
{
	unsigned long value = HX711_get_value(times);
	
	// 防止除零错误
	if (SCALE <= 0.0f) {
		ESP_LOGE(TAG, "SCALE值无效: %.6f，请重新校准传感器", SCALE);
		return 0.0f;
	}
	
	float units = (float)value / SCALE;
	
	// 检查是否为无穷大或NaN
	if (isinf(units) || isnan(units)) {
		ESP_LOGE(TAG, "重量读数异常: value=%ld, SCALE=%.6f, units=%.2f", value, SCALE, units);
		return 0.0f;
	}
	
	// 限制合理范围
	if (units < -10000.0f || units > 10000.0f) {
		ESP_LOGW(TAG, "重量读数超出合理范围: %.2fg，可能传感器异常", units);
		return 0.0f;
	}
	
	return units;
}

void HX711_tare( ) 
{
	ESP_LOGI(TAG, "===================== START TARE ====================");
	unsigned long sum = 0; 
	sum = HX711_read_average(20);
	
	// 验证offset值的合理性
	if (sum == 0 || sum > 0x7FFFFF) {
		ESP_LOGE(TAG, "Tare读数异常: %ld，可能传感器连接有问题", sum);
		return;
	}
	
	HX711_set_offset(sum);
	ESP_LOGI(TAG, "===================== END TARE: %ld ====================", sum);
}

void HX711_set_scale(float scale ) 
{
	SCALE = scale;
}

float HX711_get_scale()
 {
	return SCALE;
}

void HX711_set_offset(unsigned long offset)
 {
	OFFSET = offset;
}

unsigned long HX711_get_offset(unsigned long offset) 
{
	return OFFSET;
}

void HX711_power_down() 
{
	gpio_set_level(GPIO_PD_SCK, LOW);
	ets_delay_us(CLOCK_DELAY_US);
	gpio_set_level(GPIO_PD_SCK, HIGH);
	ets_delay_us(CLOCK_DELAY_US);
}

void HX711_power_up() 
{
	gpio_set_level(GPIO_PD_SCK, LOW);
}

// 添加重置校准函数
void HX711_reset_calibration(void)
{
    OFFSET = 0;
    SCALE = 1.0f;
    ESP_LOGI(TAG, "HX711校准已重置");
}

// 添加获取当前校准状态的函数
void HX711_print_calibration_status(void)
{
    ESP_LOGI(TAG, "HX711校准状态:");
    ESP_LOGI(TAG, "- OFFSET: %ld", OFFSET);
    ESP_LOGI(TAG, "- SCALE: %.6f", SCALE);
    ESP_LOGI(TAG, "- GAIN: %d", GAIN);
}

void weight_reading_task(void* arg)
{
    ESP_LOGI(TAG, "开始初始化HX711重量传感器...");
    HX711_init(GPIO_DATA, GPIO_SCLK, eGAIN_128); 
    
    // 等待传感器稳定
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // 检查传感器是否就绪
    if (!HX711_is_ready()) {
        ESP_LOGE(TAG, "HX711传感器未就绪，请检查连接");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "HX711传感器初始化成功");

    // 第一步：清零（无物体）
    ESP_LOGI(TAG, "开始去皮校准，请确保传感器上没有物体...");
    vTaskDelay(pdMS_TO_TICKS(2000));  // 等待2秒确保稳定
    
    HX711_tare();
    
    // 验证tare是否成功
    if (OFFSET == 0) {
        ESP_LOGE(TAG, "去皮失败，OFFSET为0");
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "完成去皮,请放上一个已知重量的物体(如500g水)...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // 等你手动放上去

    // 第二步：读数并设置校准比例
    unsigned long raw = HX711_get_value(10);  // 获取校准时的读数
    ESP_LOGI(TAG, "校准物体原始读数: %ld", raw);
    
    if (raw == 0) {
        ESP_LOGE(TAG, "校准物体读数为0，请检查传感器和物体");
        vTaskDelete(NULL);
        return;
    }
    
    float known_weight_g = 90.0f;
    float scale = (float)raw / known_weight_g;
    
    // 验证scale值的合理性
    if (scale <= 0.0f || scale > 1000000.0f) {
        ESP_LOGE(TAG, "计算出的SCALE值异常: %.6f，请检查已知重量设置", scale);
        vTaskDelete(NULL);
        return;
    }
    
    HX711_set_scale(scale);
    ESP_LOGI(TAG, "设置scale = %.2f 已知重量 = %.2f g raw = %ld", scale, known_weight_g, raw);
    
    // 测试校准结果
    float test_weight = HX711_get_units(5);
    ESP_LOGI(TAG, "校准测试: 读取重量 = %.2fg (期望: %.2fg)", test_weight, known_weight_g);

    // 循环读取重量（单位：克）
    float weight;
    float last_reported_weight = 0.0f;
    const float REPORT_THRESHOLD = 20.0f;
    const char *device_id = device_manager_get_device_id();
    
    ESP_LOGI(TAG, "开始重量监控循环...");
    while (1)
    {
        weight = HX711_get_units(AVG_SAMPLES);
        
        // 添加调试信息
        if (weight != 0.0f) {
            ESP_LOGI(TAG, "******* weight = %.2fg *********", weight);
        } else {
            ESP_LOGW(TAG, "重量读数为0，可能传感器异常");
        }
        
        if (fabs(weight - last_reported_weight) >= REPORT_THRESHOLD) {
            // 只保留一位小数
            float rounded_weight = roundf(weight * 10.0f) / 10.0f;
            ESP_LOGI(TAG, "检测到粮桶重量变化，自动上报: %.1fg", rounded_weight);
            // WebSocket上报粮桶重量
            extern esp_websocket_client_handle_t g_ws_client; // 声明全局WebSocket句柄
            if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                char ws_msg[128];
                snprintf(ws_msg, sizeof(ws_msg),
                    "{\"type\":\"grain_weight\",\"device_id\":\"%s\",\"grain_weight\":%.1f}",
                    device_id, rounded_weight);
                esp_websocket_client_send_text(g_ws_client, ws_msg, strlen(ws_msg), portMAX_DELAY);
                ESP_LOGI(TAG, "WebSocket上报粮桶重量: %s", ws_msg);
            } else {
                ESP_LOGW(TAG, "WebSocket未连接，无法上报粮桶重量");
            }
            last_reported_weight = rounded_weight;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
	vTaskDelete(NULL);
}

void initialise_weight_sensor(void)
{
    ESP_LOGI(TAG, "****************** Initialing weight sensor **********");
    xTaskCreatePinnedToCore(weight_reading_task, "weight_reading_task", 4096, NULL, 1, NULL,0);
}