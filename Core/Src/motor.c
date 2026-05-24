/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : motor.c
  * 功能描述          : 
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：
  * 创建时间：
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#include "motor.h"
#include "cmsis_os.h"
#include "tim.h"

//任务函数定义
void motor_run_f(void const * argument)
{

  for(;;)
  {
    osDelay(1);
  }
 
}

//任务函数定义
void motor_sensor_f(void const * argument)
{
  
  for(;;)
  {
    osDelay(1);
  }
  
}

//初始化电机
void motor_init()
{
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1|TIM_CHANNEL_2);
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1|TIM_CHANNEL_2, 0);
	
	HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
	__HAL_TIM_SET_COUNTER(&htim2, 0);
}

//设置电机速度
uint16_t motor_set_speed(int16_t speed)
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
void motor_brake()
{
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, PWM_PERIOD);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PWM_PERIOD);
}

//获取编码器计数值(32位)
int32_t motor_get_encoder()
{
  return (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
}
