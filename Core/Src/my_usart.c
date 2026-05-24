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
  * 作    者：向思嘉向思嘉
  * 创建时间：2026.5.23
  * 修改记录：用了串口1的发数据，用来调试传感器的数据是否正确的（周南全，2026.5.23）
  ******************************************************************************
  */
/* USER CODE END Header */

#include "my_usart.h"
#include "cmsis_os.h"
#include "angle_sensor.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

//任务函数定义
void usart1_tx_f(void const * argument)
{
	Sensor_Data_Typedef sensor_data;
	char buf[128];
	
  for(;;)
  {
     vTaskDelay(pdMS_TO_TICKS(100));

     if (xQueuePeek(xSensorQueue, &sensor_data, 0) == pdPASS)
     {
         // 打印角度和角速度
         sprintf(buf, "a1:%.1f  w1:%.1f   | a2:%.1f  w2:%.1f  \r\n",
                 sensor_data.angle1, sensor_data.angular_velocity1,
                 sensor_data.angle2, sensor_data.angular_velocity2);
         
         HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
     }
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
