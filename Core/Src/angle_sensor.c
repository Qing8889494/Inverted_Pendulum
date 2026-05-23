/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : angle_sensor.c
  * 功能描述          : ADC角度传感器数据采集
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：周南全
  * 创建时间：2026年5月23日
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#include "angle_sensor.h"
#include "cmsis_os.h"
#include "LED.h"

//任务函数定义
void angle_sensor_f(void const * argument)
{
  
  for(;;)
  {
		LED_Toggle(LED1_Pin);
    osDelay(500);
		LED_Toggle(LED2_Pin);
    osDelay(500);
		LED_Toggle(LED3_Pin);
    osDelay(500);
  }
 
}
