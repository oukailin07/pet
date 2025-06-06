#ifndef MOTOR_H
#define MOTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include <inttypes.h>

enum motor_ctr_cmd//电机控制状态
{
    MOTOR_STOP = 0, //停止
    MOTOR_FORWARD, //正转
    MOTOR_BACKWARD,//反转
    MOTOR_BRAKE,//刹车
};

void motor_control(char motor_cmd);//电机控制函数
void motor_init(void);//电机初始化函数

#endif