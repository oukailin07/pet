#include "HX711.h"
#include "esp_log.h"
#include <rom/ets_sys.h>
#include <math.h>
#include "esp_websocket_client.h"
#include "device_manager.h"
#include "websocket_client.h"
#include "key.h"
#include "app_sr.h"

#define HIGH 1
#define LOW 0
#define CLOCK_DELAY_US 20
#define AVG_SAMPLES 10
#define REPORT_THRESHOLD 5.0f  // 5g threshold for reporting weight changes

#define DEBUGTAG "HX711"

static gpio_num_t GPIO_PD_SCK = GPIO_NUM_18;	// Power Down and Serial Clock Input Pin
static gpio_num_t GPIO_DOUT = GPIO_NUM_17;		// Serial Data Output Pin
static HX711_GAIN GAIN = eGAIN_128;		// amplification factor
static unsigned long OFFSET = 0;	// used for tare weight
static float SCALE = 1;	// used to return weight in grams, kg, ounces, whatever
static const char* TAG = "HX711_TEST";

// Global variables for weight reporting
static float last_reported_weight = 0.0f;

static bool hx711_calibrated = false;
static hx711_calib_state_t hx711_calib_state = HX711_CALIB_NOT_CALIBRATED;
static TaskHandle_t calib_task_handle = NULL;

// 校准任务
static void hx711_calibration_task(void *arg) {
    while (!hx711_calibrated) {
        if (hx711_calib_state == HX711_CALIB_NOT_CALIBRATED) {
            printf("[TTS] 称重模块未校准，请放入90克的进行校准，放入后请短按按键或者请说已放入。\n");
        }
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
    vTaskDelete(NULL);
}

void hx711_start_calibration_task(void) {
    if (calib_task_handle == NULL) {
        xTaskCreatePinnedToCore(hx711_calibration_task, "hx711_calib_task", 2048, NULL, 2, &calib_task_handle, 1);
    }
}

// 外部触发校准（按键/语音）
void hx711_trigger_calibration(void) {
    if (hx711_calib_state == HX711_CALIB_DONE) {
        printf("[TTS] 称重模块已校准，无需重复校准。\n");
        return;
    }
    hx711_calib_state = HX711_CALIB_IN_PROGRESS;
    printf("[TTS] 检测到校准触发，正在校准...\n");
    // 实际校准流程
    HX711_tare();
    vTaskDelay(pdMS_TO_TICKS(1000));
    unsigned long raw = HX711_get_value(10);
    float known_weight_g = 90.0f;
    if (raw < 10000) { // 简单判断是否有物体
        printf("[TTS] 校准失败，请确保已放入90克的砝码。\n");
        hx711_calib_state = HX711_CALIB_NOT_CALIBRATED;
        return;
    }
    float scale = raw / known_weight_g;
    HX711_set_scale(scale);
    hx711_calibrated = true;
    hx711_calib_state = HX711_CALIB_DONE;
    printf("[TTS] 校准成功，可以正常使用。\n");
}

hx711_calib_state_t hx711_get_calib_state(void) {
    return hx711_calib_state;
}

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
// wait for the chip to become ready
while (HX711_is_ready()) 
{
vTaskDelay(10 / portTICK_PERIOD_MS);
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

value =value^0x800000;

return (value);
}



unsigned long  HX711_read_average(char times) 
{
	//ESP_LOGI(DEBUGTAG, "===================== READ AVERAGE START ====================");
	//ESP_LOGI(DEBUGTAG, "===================== READ AVERAGE START ====================");
unsigned long sum = 0;
for (char i = 0; i < times; i++) 
{
sum += HX711_read();
}
	//ESP_LOGI(DEBUGTAG, "===================== READ AVERAGE END : %ld ====================",(sum / times));
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
return HX711_get_value(times) / SCALE;
}

void HX711_tare( ) 
{
	ESP_LOGI(DEBUGTAG, "===================== START TARE ====================");
	//ESP_LOGI(DEBUGTAG, "===================== START TARE ====================");
unsigned long sum = 0; 
sum = HX711_read_average(20);
HX711_set_offset(sum);
	ESP_LOGI(DEBUGTAG, "===================== END TARE: %ld ====================",sum);
	//ESP_LOGI(DEBUGTAG, "===================== END TARE: %ld ====================",sum);
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

// 修改weight_reading_task，启动时检测校准状态
void weight_reading_task(void* arg)
{
    HX711_init(GPIO_DATA, GPIO_SCLK, eGAIN_128);
    hx711_calibrated = false;
    hx711_calib_state = HX711_CALIB_NOT_CALIBRATED;
    hx711_start_calibration_task();
    // 等待校准完成
    while (!hx711_calibrated) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    // 校准完成后，进入正常称重逻辑
    float weight;
    while (1)
    {
        weight = HX711_get_units(AVG_SAMPLES);
        if (weight != 0.0f) {
            ESP_LOGI(TAG, "******* weight = %.2fg *********", weight);
        } else {
            ESP_LOGW(TAG, "重量读数为0，可能传感器异常");
        }
        if (fabs(weight - last_reported_weight) >= REPORT_THRESHOLD) {
            float rounded_weight = roundf(weight * 10.0f) / 10.0f;
            ESP_LOGI(TAG, "检测到粮桶重量变化，自动上报: %.1fg", rounded_weight);
            if (g_ws_client && esp_websocket_client_is_connected(g_ws_client)) {
                const char* device_id = device_manager_get_device_id();
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