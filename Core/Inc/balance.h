/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : balance.h
  * 功能描述          : 结构体以及任务函数声明
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：高愉坤
  * 创建时间：
  * 使用说明：
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __BALANCE_H
#define __BALANCE_H

//任务函数声明
void balance_f(void const * argument);
void set_f(void const * argument);
//其它函数声明
typedef struct {
	float Target;
	float Actual;
	float Out;
	
	float Kp;
	float Ki;
	float Kd;
	
	float Error0;
	float Error1;
	float ErrorInt;
	
	float OutMax;
	float OutMin;
} PID_t;

#endif

// 完成pid调参之后使用

// #ifndef __BALANCE_H
//  #define __BALANCE_H

//  #include <stdint.h>

//  void balance_f(void const * argument);
//  void set_f(void const * argument);

//  typedef struct {
//      float Target;
//      float Actual;
//      float Out;
//      float Kp, Ki, Kd;
//      float Error0, Error1, ErrorInt;
//      float OutMax, OutMin;
//  } PID_t;

//  #endif

