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

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


void usart1_tx_f(void const * argument);
void usart1_rx_f(void const * argument);
void usart2_tx_f(void const * argument);
void usart2_rx_f(void const * argument);
void usart3_tx_f(void const * argument);
void usart3_rx_f(void const * argument);

/* -----------------------------------------------------------------------
 * 初始化函数
 * 必须在 osKernelStart() 之前调用一次。
 * 完成：创建 xPIDQueue + 创建 PIDUpdateTask。
 * 原 USART3_InitQueue() 请替换为本函数：
 *   USART_Init();
 * ----------------------------------------------------------------------- */
void USART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __MY_USART_H */
