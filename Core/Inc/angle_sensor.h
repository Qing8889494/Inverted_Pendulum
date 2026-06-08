/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : angle_sensor.h
  * 功能描述          : 传感器采集的结构声明，数据结构和队列的定义
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：周南全
  * 创建时间：2026年5月23日
  * 使用说明：包含头文件即可读取队列的数据，参考usart1_tx_f函数的时候方法，直接读取队列中的值就行了
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __ANGLE_SENSOR_H
#define __ANGLE_SENSOR_H

#include "main.h"
#include "adc.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

// 12位ADC量程
#define ADC_MAX        4095.0f
// 角度量程
#define FULL_ANGLE     360.0f

// 垂直校准值
#define A1_VERTICAL_MID  3000.0f
#define A2_VERTICAL_MID  2730.0f

//采样周期
#define SAMPLE_PERIOD   0.005f

// 外部声明
extern volatile uint16_t adc_raw[];

// 传感器数据结构体
typedef struct {
    float angle1;  // 传感器1(PA2) 滤波后值
    float angle2;  // 传感器2(PA4) 滤波后值
		float angular_velocity1;	//传感器1 角速度
		float angular_velocity2;	//传感器2 角速度
} Sensor_Data_Typedef;

// 队列句柄
extern QueueHandle_t xSensorQueue;

// 读取传感器数据
void ADC_Sensor_Read(Sensor_Data_Typedef *data);

//任务函数声明
void angle_sensor_f(void const * argument);
//其它函数声明

#endif
