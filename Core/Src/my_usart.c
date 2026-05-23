/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_usart.c
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

#include "my_usart.h"
#include "cmsis_os.h"

//任务函数定义
void usart1_tx_f(void const * argument)
{
 
  for(;;)
  {
    osDelay(1);
  }
  
}

//任务函数定义
void usart1_rx_f(void const * argument)
{

  for(;;)
  {
    osDelay(1);
  }

}

//任务函数定义
void usart2_tx_f(void const * argument)
{
 
  for(;;)
  {
    osDelay(1);
  }
  
}


//任务函数定义
void usart2_rx_f(void const * argument)
{

  for(;;)
  {
    osDelay(1);
  }
  
}

//任务函数定义
void usart3_tx_f(void const * argument)
{
  
  for(;;)
  {
    osDelay(1);
  }
  
}


//任务函数定义
void usart3_rx_f(void const * argument)
{

  for(;;)
  {
    osDelay(1);
  }
 
}
