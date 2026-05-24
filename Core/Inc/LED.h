/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : LED.h
  * 功能描述          : 调试LED驱动函数，提供LED点亮/熄灭/翻转接口，供全工程调用
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：周南全
  * 创建时间：2026年5月23日
  * 使用说明：包含头文件后，任意任务可直接调用LED调试函数
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LED_H
#define __LED_H

#include "main.h"

// 定义LED引脚
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOD

#define LED2_Pin GPIO_PIN_12
#define LED2_GPIO_Port GPIOD

#define LED3_Pin GPIO_PIN_6
#define LED3_GPIO_Port GPIOC

// 调试LED函数声明
void LED_On(uint16_t LED_Pin);    // 点亮指定LED
void LED_Off(uint16_t LED_Pin);   // 熄灭指定LED
void LED_Toggle(uint16_t LED_Pin);// 翻转LED状态

#endif
