#ifndef KEY_H
#define KEY_H



#define GPIO4_OUTPUT_IO   4
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<GPIO4_OUTPUT_IO)
#define GPIO0_INPUT_IO     0
#define GPIO_INPUT_PIN_SEL   (1ULL<<GPIO0_INPUT_IO)
#define ESP_INTR_FLAG_DEFAULT 0
typedef enum {
    BUTTON_EVENT_SINGLE_CLICK,
    BUTTON_EVENT_DOUBLE_CLICK,
    BUTTON_EVENT_LONG_PRESS
} button_event_t;
void key_init(void);
void button_event_callback(button_event_t event); // 用户自己实现
#endif