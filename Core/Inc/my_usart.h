/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_usart.h
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

#ifndef __MY_USART_H
#define __MY_USART_H

//任务函数声明
void usart1_tx_f(void const * argument);
void usart1_rx_f(void const * argument);
void usart2_tx_f(void const * argument);
void usart2_rx_f(void const * argument);
void usart3_tx_f(void const * argument);
void usart3_rx_f(void const * argument);
//其它函数声明

#endif
