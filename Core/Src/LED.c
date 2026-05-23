/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : LED.c
  * 功能描述          : 调试LED驱动实现，用于系统调试、状态指示
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

#include "led.h"

// 点亮LED
void LED_On(uint16_t LED_Pin)
{
    if(LED_Pin == LED1_Pin)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    else if(LED_Pin == LED2_Pin)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    else if(LED_Pin == LED3_Pin)
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

// 熄灭LED
void LED_Off(uint16_t LED_Pin)
{
    if(LED_Pin == LED1_Pin)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    else if(LED_Pin == LED2_Pin)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    else if(LED_Pin == LED3_Pin)
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

// 翻转LED
void LED_Toggle(uint16_t LED_Pin)
{
    if(LED_Pin == LED1_Pin)
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED_Pin);
    else if(LED_Pin == LED2_Pin)
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED_Pin);
    else if(LED_Pin == LED3_Pin)
        HAL_GPIO_TogglePin(LED3_GPIO_Port, LED_Pin);
}
