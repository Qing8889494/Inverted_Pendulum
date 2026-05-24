/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : motor.c
  * 功能描述          : 根据电机命令队列的电机速度来控制PWM输出；
	*											根据电机编码器获得的数据发送电机的当前转速。
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：韩庆
  * 创建时间：2026.5.23
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#include "motor.h"


QueueHandle_t	xMotorCmdQueue;				//接收命令队列
QueueHandle_t xMotorFeedbackQueue;	//发送反馈队列

//任务函数定义
void motor_run_f(void const * argument)
{
	//初始化
	MotorCmd_t cmd;
	motor_init();
	
  for(;;)
  {
		//等待接收命令
		if (xQueueReceive(xMotorCmdQueue, &cmd, portMAX_DELAY) == pdTRUE)
		{
			//设置目标速度
			motor_set_speed(cmd.target_speed);
		}
  }
 
}

//任务函数定义
void motor_sensor_f(void const * argument)
{
  MotorFeedback_t feedback;
	int32_t		last_cnt	= 0;
	uint32_t	last_time	= 0;
	
	uint32_t	now;
	uint32_t	dt;
	int32_t delta;
	
  for(;;)
  {
		//读取当前编码器值
		feedback.encoder_pos	= (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
		feedback.timestamp_ms	= HAL_GetTick();
		
		//计算转速
		now	= feedback.timestamp_ms;
		dt	= now - last_time;
		if(dt>=10)
		{
			delta	= feedback.encoder_pos-last_cnt;
			feedback.speed_rpm	= (float)delta * 60000.0f / (ENCODER_PPR * dt);
			last_cnt	= feedback.encoder_pos;
			last_time	= now;
		}
		else{
			
		}
		if ( xQueueSend(xMotorFeedbackQueue, &feedback, pdMS_TO_TICKS(10)) != pdPASS)
		{
			
		}
		
		osDelay(10);
  }
  
}

//初始化电机
void motor_init(void)
{

	//初始化PWM
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1|TIM_CHANNEL_2);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1|TIM_CHANNEL_2, 0);
	//初始化编码器
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
}

//设置电机速度
void motor_set_speed(int16_t speed)
{
	//电机速度范围为-1000~1000
	uint16_t	duty;		//占空比的值
	if (speed>0)
	{
		duty = (uint16_t)((int32_t)speed * PWM_PERIOD / 10000);
		if (duty > PWM_PERIOD) duty = PWM_PERIOD;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	}
	if(speed<0)
	{
		duty = (uint16_t)((int32_t)(-speed) * PWM_PERIOD / 10000);
		if (duty > PWM_PERIOD) duty = PWM_PERIOD;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
	}
	else
	{
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	}
}

//电机刹车
void motor_brake(void)
{
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PWM_PERIOD);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PWM_PERIOD);
}
