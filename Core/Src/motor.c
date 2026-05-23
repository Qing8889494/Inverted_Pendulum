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
