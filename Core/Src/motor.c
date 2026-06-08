/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : motor.c
  * 功能描述          : 根据电机命令队列的电机速度来控制PWM输出；(能让电机空载输
												出转速的最小speed值是550)
	*											根据电机编码器获得的数据发送电机的当前转速。
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：韩庆
  * 创建时间：2026.5.23
  * 修改记录：完成基本代码逻辑，还未测试（2026.5.24）
	*						
  ******************************************************************************
  */
/* USER CODE END Header */

#include "motor.h"
#include "led.h"
#include "my_oled.h"
#include "angle_sensor.h"


QueueHandle_t	xMotorCmdQueue;				//接收命令队列
QueueHandle_t xMotorFeedbackQueue;	//发送反馈队列



//任务函数定义
void motor_run_f(void const * argument)
{
	MotorCmd_t cmd;
	//初始化
	motor_init();
	
  for(;;)
  {
		//等待接收命令
		if (xQueueReceive(xMotorCmdQueue, &cmd, portMAX_DELAY) == pdTRUE)
		{
			//设置目标速度
			motor_set_speed(cmd.target_speed);
			
			//oled_show("speed", (float)cmd.target_speed);
		}
  }
 
}

//任务函数定义
void motor_sensor_f(void const * argument)
{
  MotorFeedback_t feedback;
	int32_t		last_cnt	= 0;
	uint32_t	last_time	= 0;
	int16_t		last_rpm	= 0;
	
	uint32_t	now;
	uint32_t	dt;
	int32_t delta;
	int32_t raw_angle;
	
  for(;;)
  {
		//读取当前编码器值
		feedback.encoder_pos	= (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
		feedback.timestamp_ms	= HAL_GetTick();
		
		//计算绝对角度
		raw_angle		= feedback.encoder_pos % ENCODER_PPR;
		if (raw_angle<0) raw_angle	+= ENCODER_PPR;
		feedback.angle_deg	= (float)raw_angle * 360.0f / ENCODER_PPR;
		
		//计算转速
		now	= feedback.timestamp_ms;
		dt	= now - last_time;
		if(dt>=10)
		{
			delta	= feedback.encoder_pos-last_cnt;
			feedback.speed_rpm	= (float)delta * 60000.0f / (ENCODER_PPR * dt);
			last_rpm	= feedback.speed_rpm;
			last_cnt	= feedback.encoder_pos;
			last_time	= now;
		}
		else{
			//时间间隔太短，保持上一次转速
			feedback.speed_rpm	= last_rpm;
		}
		if ( xQueueOverwrite(xMotorFeedbackQueue, &feedback) != pdPASS)
		{
		}
		
		osDelay(5);
  }
  
}

//初始化电机
void motor_init(void)
{

	//初始化PWM
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);

	//初始化编码器
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
}

//设置电机速度
void motor_set_speed(int16_t speed)
{
	//电机速度范围为-1000~1000
	uint16_t	duty;		//占空比的值
	if (speed<0)
	{
		speed = speed - 550;
		duty = (uint16_t)((int32_t)-speed * PWM_PERIOD / 1000);
		if (duty > PWM_PERIOD) duty = PWM_PERIOD;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, duty);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	}
	else if(speed>0)
	{
		speed = speed + 550;
		duty = (uint16_t)((int32_t)(speed) * PWM_PERIOD / 1000);
		if (duty > PWM_PERIOD) duty = PWM_PERIOD;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, duty);
	}
	else
	{
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
	}
<<<<<<< HEAD
//	oled_show("pwm", (float)duty);
=======
>>>>>>> a48d36b05e505bab6b1568f4b1b9271d348de205
}

//电机刹车
void motor_brake(void)
{
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PWM_PERIOD);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PWM_PERIOD);
}

//获取编码器计数值(32位)
int32_t motor_get_encoder()
{
  return (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
}
