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
    * 作    者：向思嘉
    * 创建时间：2026.5.23
    * 修改记录：用了串口1的发数据，用来调试传感器的数据是否正确的（周南全，2026.5.23）
      *                        用串口2的收数据，来接收电脑端发送的电机速度指令：@v 300
      *                        增加串口2 SerialPlot 波形发送（向思嘉，2026.5.25）
    *           增加串口3 PID 参数接收（向思嘉，2026.5.25）
      *                        用串口1的收数据，来接收电脑端发送的电机速度指令：韩庆，2026.5.24
      *                        用串口1的发数据，来调试电机的编码器数据（2026.5.28）
      *                        优化了串口2发送波形的代码，把串口3接收到的PID参数传到了flash里面，防止掉电重置（周南全 2026.6.1）
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
  #include "led.h"
  #include "my_oled.h"
  #include "balance.h"
  #include "stm32f4xx_hal_flash.h"

  #define PID_PARAM_FLASH_ADDR    0x0807F000U
  #define PID_FLASH_MAGIC         0xDEADBEEFU

  typedef struct
  {
      uint32_t magic;
      float    inner_kp;
      float    inner_ki;
      float    inner_kd;
      float    outer_kp;
      float    outer_ki;
      float    outer_kd;
  } PID_Store_t;

  extern PID_t AnglePID;
  extern PID_t LocationPID;
  extern volatile int16_t g_last_motor_speed;

  static void PID_Flash_Write(PID_Store_t *pParam)
  {
      HAL_FLASH_Unlock();

      FLASH_EraseInitTypeDef eraseInit = {0};
      uint32_t pageError = 0;
      eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
      eraseInit.Sector       = FLASH_SECTOR_7;
      eraseInit.NbSectors    = 1;
      eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
      HAL_FLASHEx_Erase(&eraseInit, &pageError);

      uint32_t *pSrc = (uint32_t *)pParam;
      for (uint16_t i = 0; i < sizeof(PID_Store_t) / 4; i++)
      {
          HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PID_PARAM_FLASH_ADDR + i * 4, pSrc[i]);
      }

      HAL_FLASH_Lock();
  }

  static void PID_Flash_Read(PID_Store_t *pParam)
  {
      memcpy(pParam, (void *)PID_PARAM_FLASH_ADDR, sizeof(PID_Store_t));
  }

  void PID_Load_From_Flash(void)
  {
      PID_Store_t tmp;
      PID_Flash_Read(&tmp);

      if (tmp.magic != PID_FLASH_MAGIC)
          return;

      AnglePID.Kp    = tmp.inner_kp;
      AnglePID.Ki    = tmp.inner_ki;
      AnglePID.Kd    = tmp.inner_kd;
      LocationPID.Kp = tmp.outer_kp;
      LocationPID.Ki = tmp.outer_ki;
      LocationPID.Kd = tmp.outer_kd;
  }

  void PID_Save_To_Flash(void)
  {
      PID_Store_t tmp;
      tmp.magic     = PID_FLASH_MAGIC;
      tmp.inner_kp  = AnglePID.Kp;
      tmp.inner_ki  = AnglePID.Ki;
      tmp.inner_kd  = AnglePID.Kd;
      tmp.outer_kp  = LocationPID.Kp;
      tmp.outer_ki  = LocationPID.Ki;
      tmp.outer_kd  = LocationPID.Kd;
      PID_Flash_Write(&tmp);
  }


  void usart1_tx_f(void const *argument)
  {
      MotorFeedback_t sensor_motor;
      char buf[128];

      for (;;)
      {
          if (xQueueReceive(xMotorFeedbackQueue, &sensor_motor, pdMS_TO_TICKS(1000)) == pdPASS)
          {
              sprintf(buf, "motor_rpm: %.2f , motor_angle: %.1f , motor_step: %d\r\n",
                      sensor_motor.speed_rpm,
                      sensor_motor.angle_deg,
                      sensor_motor.encoder_pos);
              HAL_UART_Transmit(&huart1, (uint8_t *)buf, strlen(buf), 100);
          }
      }
  }


  #define UART1_RX_BUF_SIZE 32
  static uint8_t  uart1_rx_buf[UART1_RX_BUF_SIZE];
  static uint16_t uart1_rx_idx = 0;

  void usart1_rx_f(void const *argument)
  {
      uint8_t    rx_char;
      int16_t    speed = 0;
      MotorCmd_t motor_cmd;

      for (;;)
      {
          if (HAL_UART_Receive(&huart1, &rx_char, 1, 1) == HAL_OK)
          {
              switch (rx_char)
              {
                  case '@':
                      uart1_rx_idx = 0;
                      uart1_rx_buf[uart1_rx_idx++] = rx_char;
                      break;

                  case '\r':
                  case '\n':
                      uart1_rx_buf[uart1_rx_idx] = '\0';

                      if (uart1_rx_idx >= 4 &&
                          uart1_rx_buf[0] == '@' &&
                          uart1_rx_buf[1] == 'v' &&
                          uart1_rx_buf[2] == ' ')
                      {
                          speed = (int16_t)atoi((char *)&uart1_rx_buf[3]);
                          speed = (speed > 1000) ? 1000 : (speed < -1000) ? -1000 : speed;

                          motor_cmd.target_speed = speed;
                          motor_cmd.target_angle = 0;

                          if (xQueueSend(xMotorCmdQueue, &motor_cmd, pdMS_TO_TICKS(10)) == pdPASS)
                          {
                              char ack_buf[48];
                              sprintf(ack_buf, "Success: Motor speed set to %d\r\n", speed);
                              HAL_UART_Transmit(&huart1, (uint8_t *)ack_buf, strlen(ack_buf), 100);
                          }
                          else
                          {
                              HAL_UART_Transmit(&huart1,
                                  (uint8_t *)"Error: Queue full\r\n", 19, 100);
                          }
                      }
                      else
                      {
                          HAL_UART_Transmit(&huart1,
                              (uint8_t *)"Error: Invalid format (use @v +-xxx)\r\n", 38, 100);
                      }
                      uart1_rx_idx = 0;
                      break;

                  default:
                      if (uart1_rx_idx < UART1_RX_BUF_SIZE - 1)
                          uart1_rx_buf[uart1_rx_idx++] = rx_char;
                      else
                          uart1_rx_idx = 0;
                      break;
              }
          }
          osDelay(1);
      }
  }


  void usart2_tx_f(void const * argument)
  {
      MotorFeedback_t sensor_data;
			Sensor_Data_Typedef angle_data;
      char tx_buf[128];
      char *ptr;
	  
      for(;;)
      {
          if (xQueuePeek(xMotorFeedbackQueue, &sensor_data, pdMS_TO_TICKS(50)) != pdPASS || xQueuePeek(xSensorQueue, &angle_data, pdMS_TO_TICKS(50))!= pdPASS)
          {
              continue;
          }
		 
					//LED_On(LED2_Pin);
          ptr = tx_buf;
          ptr += sprintf(ptr, "%.2f", angle_data.angle2);
          *ptr++ = ',';
          ptr += sprintf(ptr, "%.2f", sensor_data.angle_deg);
          *ptr++ = ',';
		  ptr += sprintf(ptr, "%d", 60);
          *ptr++ = '\r';
          *ptr++ = '\n';
          *ptr = '\0';

           HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, ptr - tx_buf, 100);

          osDelay(20);
      }
  }


  void usart2_rx_f(void const *argument)
  {
      for (;;) { osDelay(1); }
  }

  void usart3_tx_f(void const *argument)
  {
      for (;;) { osDelay(1); }
  }


  typedef struct {
      float inner_kp;
      float inner_ki;
      float inner_kd;
      float outer_kp;
      float outer_ki;
      float outer_kd;
  } PID_Params_t;

  static QueueHandle_t xPIDQueue = NULL;

  void usart3_rx_f(void const *argument)
  {
      uint8_t      rx_char;
      uint8_t      rx_buf[64];
      uint8_t      idx = 0;
      PID_Params_t pid;

      for (;;)
      {
          if (HAL_UART_Receive(&huart3, &rx_char, 1, 100) == HAL_OK)
          {
							LED_Off(LED1_Pin);
              if (rx_char == '\n')
              {
                  rx_buf[idx] = '\0';

                  if (idx > 0 && xPIDQueue != NULL)
                  {
                      if (sscanf((char *)rx_buf, "%f,%f,%f,%f,%f,%f",
                                 &pid.inner_kp, &pid.inner_ki, &pid.inner_kd,
                                 &pid.outer_kp, &pid.outer_ki, &pid.outer_kd) == 6)
                      {
                          xQueueOverwrite(xPIDQueue, &pid);
												LED_On(LED1_Pin);
                      }
                  }
                  idx = 0;
              }
              else if (rx_char != '\r')
              {
                  if (idx < sizeof(rx_buf) - 1)
                      rx_buf[idx++] = rx_char;
                  else
                  {
                      idx = 0;
                  }
              }
          }
      }
  }


  static void PIDUpdateTask(void *pvParameters)
  {
      PID_Params_t new_pid;

      for (;;)
      {
          if (xQueueReceive(xPIDQueue, &new_pid, pdMS_TO_TICKS(100)) == pdTRUE)
          {
              taskENTER_CRITICAL();
              {
                  AnglePID.Kp    = new_pid.inner_kp;
                  AnglePID.Ki    = new_pid.inner_ki;
                  AnglePID.Kd    = new_pid.inner_kd;
                  LocationPID.Kp = new_pid.outer_kp;
                  LocationPID.Ki = new_pid.outer_ki;
                  LocationPID.Kd = new_pid.outer_kd;
              }
              taskEXIT_CRITICAL();

              PID_Save_To_Flash();
          }
      }
  }


  void USART_Init(void)
  {
      HAL_UART_Init(&huart2);
      HAL_UART_Init(&huart3);
      PID_Load_From_Flash();
  }

  void PID_Queue_Init(void)
  {
      xPIDQueue = xQueueCreate(1, sizeof(PID_Params_t));
      if (xPIDQueue == NULL)
          HAL_UART_Transmit(&huart3, (uint8_t *)"[FAIL] xPIDQueue\r\n", 18, 100);
  }

  void PID_Task_Init(void)
  {
	  HAL_UART_Transmit(&huart2, (uint8_t *)"[INIT] PID_Task_Init\r\n", 22, 100);
      BaseType_t ret = xTaskCreate(PIDUpdateTask, "PIDUpd", 512, NULL, configMAX_PRIORITIES - 1, NULL);
      if (ret != pdPASS)
          HAL_UART_Transmit(&huart2, (uint8_t *)"[FAIL] PIDUpdateTask\r\n", 22, 100);
	  else
		  HAL_UART_Transmit(&huart2, (uint8_t *)"[OK] PIDUpdateTask created\r\n", 28, 100);
  }
  