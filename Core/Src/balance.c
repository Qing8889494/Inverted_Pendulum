/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : balance.c
  * 功能描述          : 完成所有平衡算法
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：高愉坤
  * 创建时间：2026.5.24
  * 修改记录：添加了启动判断 （周南全 2026.6.1）
  ******************************************************************************
  */
 /* USER CODE BEGIN Header */
  /**
    ******************************************************************************
    * @file           : balance.c
    * @brief          : Balance control algorithm
    ******************************************************************************
    */
  /* USER CODE END Header */

  #include "balance.h"
  #include "cmsis_os.h"
  #include "angle_sensor.h"
  #include "motor.h"
  #include "led.h"
  #include "usart.h"
  #include <stdio.h>

  #define CENTER_ANGLE        57
  #define CENTER_RANGE        40
	
	#define FRICTION_COMP_LOW   50       // 中心以下的补偿力矩（阻尼大，需要更大）
  #define FRICTION_DEADBAND_LOW   5.0f  // 中心以下的死区 (°)
  #define FRICTION_COMP_HIGH  40       // 中心以上的补偿力矩（阻尼小，需要较小）
  #define FRICTION_DEADBAND_HIGH  2.0f  // 中心以上的死区 (°)

  void PID_Update(PID_t *p)
  {
      p->Error1 = p->Error0;
      p->Error0 = p->Target - p->Actual;

      if (p->Ki > 0.00001f || p->Ki < -0.00001f)
      {
          p->ErrorInt += p->Error0;
      }
      else
      {
          p->ErrorInt = 0;
      }

      p->Out = p->Kp * p->Error0
             + p->Ki * p->ErrorInt
             + p->Kd * (p->Error0 - p->Error1);

      if (p->Out > p->OutMax) {p->Out = p->OutMax;}
      if (p->Out < p->OutMin) {p->Out = p->OutMin;}

      if(p->Ki != 0)
      {
          if (p->ErrorInt > p->OutMax / p->Ki) p->ErrorInt = p->OutMax / p->Ki;
          if (p->ErrorInt < p->OutMin / p->Ki) p->ErrorInt = p->OutMin / p->Ki;
      }
  }


  PID_t AnglePID = {
      .Target = CENTER_ANGLE,
      .Kp = 2.0f,
      .Ki = 0.0f,
      .Kd = 0.8f,
      .OutMax = 450,
      .OutMin = -450,
  };

  PID_t LocationPID = {
      .Target = 0.0f,
      .Kp = 1.0f,
      .Ki = 0.02f,
      .Kd = 0.1f,
      .OutMax = 40,
      .OutMin = -40,
  };

  Sensor_Data_Typedef angle;
  MotorFeedback_t motor_sensor;
  MotorCmd_t motor_command;

  volatile uint8_t RunState = 1;
  volatile int16_t g_last_motor_speed = 0;

  void balance_f(void const * argument)
  {
      static uint16_t Count1 = 0, Count2 = 0;
      Sensor_Data_Typedef angle_local;
      MotorFeedback_t motor_local;
      MotorCmd_t motor_cmd;
		

      for(;;)
      {
          if (xQueuePeek(xSensorQueue, &angle_local, pdMS_TO_TICKS(15)) == pdTRUE) {
//              LED_Off(LED1_Pin);
          } else {
 //             LED_On(LED1_Pin);
							osDelay(1);
              continue;
          }


          if (xQueuePeek(xMotorFeedbackQueue, &motor_local, pdMS_TO_TICKS(15)) == pdTRUE) {
//              LED_Off(LED2_Pin);
          } else {
//              LED_On(LED2_Pin);
			  
			  osDelay(1);
              continue;
          }


          if (angle_local.angle2 < CENTER_ANGLE - CENTER_RANGE ||
              angle_local.angle2 > CENTER_ANGLE + CENTER_RANGE //||
//							motor_local.encoder_pos > 800 ||
//							motor_local.encoder_pos < -800
					) {
              RunState = 0;
              motor_cmd.target_speed = 0;
              g_last_motor_speed = 0;
							AnglePID.ErrorInt	=0;
							LocationPID.ErrorInt =0;
              xQueueSend(xMotorCmdQueue, &motor_cmd, 0);
              LED_Off(LED1_Pin);
								
				osDelay(1);
							osDelay(1);
              continue;
          }

          LED_On(LED1_Pin);

          Count1++;
          Count2++;

          if (Count1 >= 5) {
              Count1 = 0;
              AnglePID.Actual = angle_local.angle2;
              PID_Update(&AnglePID);
              motor_cmd.target_speed = (int16_t)AnglePID.Out;

						
						 // 当角度误差超出死区、但 PID 输出太小时，补足到最小输出
             float angle_err = AnglePID.Target - angle_local.angle2;
              if (angle_err > FRICTION_DEADBAND_LOW)
                  motor_cmd.target_speed += FRICTION_COMP_LOW;
              else if (angle_err < -FRICTION_DEADBAND_HIGH)
                  motor_cmd.target_speed -= FRICTION_COMP_HIGH;

              if(motor_cmd.target_speed == 0) {
                  LED_On(LED3_Pin);
              } else {
                  LED_Off(LED3_Pin);
              }
              g_last_motor_speed = motor_cmd.target_speed;
              xQueueSend(xMotorCmdQueue, &motor_cmd, 0);
          }

          if (Count2 >= 25) {
              Count2 = 0;
              LocationPID.Actual = motor_local.encoder_pos;
              PID_Update(&LocationPID);
              AnglePID.Target = CENTER_ANGLE - LocationPID.Out;
          }
					osDelay(1);
      }
  }

  void set_f(void const * argument)
  {
      GPIO_PinState key_now;
      static uint8_t key_last = 0;

      for(;;)
      {
          key_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8);

          if (key_last == GPIO_PIN_RESET && key_now == GPIO_PIN_SET)
          {
              osDelay(20);
              if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == GPIO_PIN_SET)
              {
                  RunState = !RunState;
              }
          }

          key_last = key_now;

          if (RunState)
          {
              LED_On(LED2_Pin);
          }
          else
          {
              LED_Off(LED2_Pin);
          }

          osDelay(1);
      }
  }

//	这段是带有自动起摆的任务函数，在完成调参之后进行使用
//  void set_f(void const * argument)
//  {
//      GPIO_PinState key_now;
//      static uint8_t key_last = 0;

//      for(;;)
//      {
//          key_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8);

//          if (key_last == GPIO_PIN_RESET && key_now == GPIO_PIN_SET)
//          {
//              osDelay(20);
//              if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == GPIO_PIN_SET)
//              {
//                  if (RunState == 0)
//                      RunState = 21;      // 按键启动 → 开始起摆
//                                      // 从21开始，先来一下向左推，让摆杆离开盲区
//                  else
//                      RunState = 0;       // 再按停止
//              }
//          }

//          key_last = key_now;

//          if (RunState)
//              LED_On(LED1_Pin);
//          else
//              LED_Off(LED1_Pin);

//          osDelay(1);
//      }
//  }

