#include "motor.h"

//电机控制
/*
TC118S 通过?两个输入信号（IA和IB）?的组合控制电机状态：
IA (A-IA)	IB (A-IB)	电机状态
高电平		低电平			正转
低电平		高电平			反转
高电平		高电平			刹车（停止）
低电平		低电平			停止（惯性滑行）
*/
void motor_control(char motor_cmd)
{
    switch (motor_cmd)
    {
        case MOTOR_STOP:
            gpio_set_level(GPIO_NUM_45, 0);//停止（惯性滑行）
            gpio_set_level(GPIO_NUM_48, 0);
            // pet_bord_state.motorState = MOTOR_STOP;
            break;
        case MOTOR_FORWARD:
            gpio_set_level(GPIO_NUM_45, 0); //正转
            gpio_set_level(GPIO_NUM_48, 1); 
            // pet_bord_state.motorState = MOTOR_FORWARD;
            break;
        case MOTOR_BACKWARD:
            gpio_set_level(GPIO_NUM_45, 1); //反转
            gpio_set_level(GPIO_NUM_48, 0); 
            // pet_bord_state.motorState = MOTOR_FORWARD;
            break;
        case MOTOR_BRAKE:
            gpio_set_level(GPIO_NUM_45, 1); //C刹车（停止）
            gpio_set_level(GPIO_NUM_48, 1); 
            // pet_bord_state.motorState = MOTOR_STOP;
            break;
        default:
            break;
    }
}


static void motor_crl_init(void)
{

        //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_NUM_45 | GPIO_NUM_48;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_45, 0); //初始状态为停止
    gpio_set_level(GPIO_NUM_48, 0); //初始状态为停止

}


void motor_init(void)
{
    motor_crl_init(); //电机控制引脚初始化
}
