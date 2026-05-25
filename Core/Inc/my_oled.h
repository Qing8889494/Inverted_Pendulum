/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_oled.h
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

#ifndef __MY_OLED_H
#define __MY_OLED_H
#include "main.h"
//任务函数声明
void oled_f(void const * argument);
//其它函数声明
void oled_show(const char *label, float value);

#endif
