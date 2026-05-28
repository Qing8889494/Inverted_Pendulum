/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : motor.h
  * 功能描述          : 
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：
  * 创建时间：
  * 使用说明：
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>
#include "cmsis_os.h"
#include "tim.h"

#define PWM_PERIOD	2099		//TIM3的自动重载值
#define ENCODER_PPR	1560		//电机转一圈编码器输出的计数数量

extern QueueHandle_t	xMotorCmdQueue;
extern QueueHandle_t xMotorFeedbackQueue;

//电机控制数据结构体
typedef struct {
	int16_t	target_speed;		//目标转速
	int16_t target_angle;		//目标角度
}MotorCmd_t;

typedef struct {
	int32_t 	encoder_pos;    // 当前编码器累计位置
	float   	speed_rpm;      // 当前转速（RPM：每分钟转数）
	uint32_t 	timestamp_ms;  // 采样时间戳（可选）
}MotorFeedback_t;

//任务函数声明
void motor_run_f(void const * argument);
void motor_sensor_f(void const * argument);
//其它函数声明
void motor_init(void);
void motor_set_speed(int16_t speed);
void motor_brake(void);
#endif
