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
	*						用串口2的收数据，来接收电脑端发送的电机速度指令：@v 300 （韩庆，2026.5.24）
  ******************************************************************************
  */
/* USER CODE END Header */

#include "my_usart.h"
#include "cmsis_os.h"
#include "angle_sensor.h"
#include "motor.h"
#include "usart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UART1_RX_BUF_SIZE 32
static uint8_t uart1_rx_buf[UART1_RX_BUF_SIZE];  // 接收缓冲
static uint8_t uart1_rx_idx = 0;                 // 缓冲索引

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

  uint8_t rx_char;          // 单次接收的字符
  int16_t speed = 0;        // 解析出的速度值
  MotorCmd_t motor_cmd;     // 电机指令结构体

  for(;;)
  {
    // 轮询接收1个字符，超时1ms（降低CPU占用）
    if (HAL_UART_Receive(&huart1, &rx_char, 1, 1) == HAL_OK)
    {
      switch(rx_char)
      {
        case '@':  // 指令起始符，重置缓冲区
          uart1_rx_idx = 0;
          uart1_rx_buf[uart1_rx_idx++] = rx_char;
          break;

        case '\r': // 回车/换行作为指令结束符
        case '\n':
          uart1_rx_buf[uart1_rx_idx] = '\0'; // 字符串结束符
          
          // 解析指令：@v 300 （格式检查：长度≥4、@v+空格开头）
          if (uart1_rx_idx >= 4 && uart1_rx_buf[0] == '@' && 
              uart1_rx_buf[1] == 'v' && uart1_rx_buf[2] == ' ')
          {
            // 提取空格后的速度数值（转换为int）
            speed = atoi((char *)&uart1_rx_buf[3]);
            
            // 速度边界检查（-1000 ~ 1000，匹配motor_set_speed的范围）
            speed = (speed > 1000) ? 1000 : (speed < -1000) ? -1000 : speed;
            
            // 封装电机指令
            motor_cmd.target_speed = speed;
            motor_cmd.target_angle = 0; // 测试用，角度暂设为0
            
            // 发送到电机命令队列（超时10ms）
            if (xQueueSend(xMotorCmdQueue, &motor_cmd, pdMS_TO_TICKS(10)) == pdPASS)
            {
              // 发送成功，返回确认信息
              char ack_buf[40];
              sprintf(ack_buf, "Success: Motor speed set to %d\r\n", speed);
              HAL_UART_Transmit(&huart1, (uint8_t *)ack_buf, strlen(ack_buf), 100);
            }
            else
            {
              // 队列满，发送失败
              HAL_UART_Transmit(&huart1, (uint8_t *)"Error: Queue full, send fail\r\n", 30, 100);
            }
          }
          else
          {
            // 指令格式错误
            HAL_UART_Transmit(&huart1, (uint8_t *)"Error: Invalid format (use @v ±xxx)\r\n", 40, 100);
          }
          uart1_rx_idx = 0; // 重置缓冲区
          break;

        default:
          // 普通字符，加入缓冲区（防止溢出）
          if (uart1_rx_idx < UART1_RX_BUF_SIZE - 1)
          {
            uart1_rx_buf[uart1_rx_idx++] = rx_char;
          }
          else
          {
            uart1_rx_idx = 0; // 缓冲区溢出，重置
          }
          break;
      }
    }
    osDelay(1); // 轻微延时，降低CPU占用
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
