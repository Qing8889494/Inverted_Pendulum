/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tim.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern	uint16_t adc_raw[ADC_CHANNEL_NUM];
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId Usart1_txHandle;
osThreadId Usart1_rxHandle;
osThreadId Usart2_txHandle;
osThreadId Usart2_rxHandle;
osThreadId Usart3_txHandle;
osThreadId Usart3_rxHandle;
osThreadId OLEDHandle;
osThreadId Angle_sensorHandle;
osThreadId Motor_runHandle;
osThreadId Motor_sensorHandle;
osThreadId BalanceHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void usart1_tx_f(void const * argument);
void usart1_rx_f(void const * argument);
void usart2_tx_f(void const * argument);
void usart2_rx_f(void const * argument);
void usart3_tx_f(void const * argument);
void usart3_rx_f(void const * argument);
void oled_f(void const * argument);
void angle_sensor_f(void const * argument);
void motor_run_f(void const * argument);
void motor_sensor_f(void const * argument);
void balance_f(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of Usart1_tx */
  osThreadDef(Usart1_tx, usart1_tx_f, osPriorityIdle, 0, 128);
  Usart1_txHandle = osThreadCreate(osThread(Usart1_tx), NULL);

  /* definition and creation of Usart1_rx */
  osThreadDef(Usart1_rx, usart1_rx_f, osPriorityIdle, 0, 128);
  Usart1_rxHandle = osThreadCreate(osThread(Usart1_rx), NULL);

  /* definition and creation of Usart2_tx */
  osThreadDef(Usart2_tx, usart2_tx_f, osPriorityIdle, 0, 128);
  Usart2_txHandle = osThreadCreate(osThread(Usart2_tx), NULL);

  /* definition and creation of Usart2_rx */
  osThreadDef(Usart2_rx, usart2_rx_f, osPriorityIdle, 0, 128);
  Usart2_rxHandle = osThreadCreate(osThread(Usart2_rx), NULL);

  /* definition and creation of Usart3_tx */
  osThreadDef(Usart3_tx, usart3_tx_f, osPriorityIdle, 0, 128);
  Usart3_txHandle = osThreadCreate(osThread(Usart3_tx), NULL);

  /* definition and creation of Usart3_rx */
  osThreadDef(Usart3_rx, usart3_rx_f, osPriorityIdle, 0, 128);
  Usart3_rxHandle = osThreadCreate(osThread(Usart3_rx), NULL);

  /* definition and creation of OLED */
  osThreadDef(OLED, oled_f, osPriorityIdle, 0, 128);
  OLEDHandle = osThreadCreate(osThread(OLED), NULL);

  /* definition and creation of Angle_sensor */
  osThreadDef(Angle_sensor, angle_sensor_f, osPriorityIdle, 0, 128);
  Angle_sensorHandle = osThreadCreate(osThread(Angle_sensor), NULL);

  /* definition and creation of Motor_run */
  osThreadDef(Motor_run, motor_run_f, osPriorityIdle, 0, 128);
  Motor_runHandle = osThreadCreate(osThread(Motor_run), NULL);

  /* definition and creation of Motor_sensor */
  osThreadDef(Motor_sensor, motor_sensor_f, osPriorityIdle, 0, 128);
  Motor_sensorHandle = osThreadCreate(osThread(Motor_sensor), NULL);

  /* definition and creation of Balance */
  osThreadDef(Balance, balance_f, osPriorityIdle, 0, 128);
  BalanceHandle = osThreadCreate(osThread(Balance), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_usart1_tx_f */
/**
* @brief Function implementing the Usart1_tx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usart1_tx_f */
void usart1_tx_f(void const * argument)
{
  /* USER CODE BEGIN usart1_tx_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usart1_tx_f */
}

/* USER CODE BEGIN Header_usart1_rx_f */
/**
* @brief Function implementing the Usart1_rx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usart1_rx_f */
void usart1_rx_f(void const * argument)
{
  /* USER CODE BEGIN usart1_rx_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usart1_rx_f */
}

/* USER CODE BEGIN Header_usart2_tx_f */
/**
* @brief Function implementing the Usart2_tx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usart2_tx_f */
void usart2_tx_f(void const * argument)
{
  /* USER CODE BEGIN usart2_tx_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usart2_tx_f */
}

/* USER CODE BEGIN Header_usart2_rx_f */
/**
* @brief Function implementing the Usart2_rx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usart2_rx_f */
void usart2_rx_f(void const * argument)
{
  /* USER CODE BEGIN usart2_rx_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usart2_rx_f */
}

/* USER CODE BEGIN Header_usart3_tx_f */
/**
* @brief Function implementing the Usart3_tx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usart3_tx_f */
void usart3_tx_f(void const * argument)
{
  /* USER CODE BEGIN usart3_tx_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usart3_tx_f */
}

/* USER CODE BEGIN Header_usart3_rx_f */
/**
* @brief Function implementing the Usart3_rx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_usart3_rx_f */
void usart3_rx_f(void const * argument)
{
  /* USER CODE BEGIN usart3_rx_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END usart3_rx_f */
}

/* USER CODE BEGIN Header_oled_f */
/**
* @brief Function implementing the OLED thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_oled_f */
void oled_f(void const * argument)
{
  /* USER CODE BEGIN oled_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END oled_f */
}

/* USER CODE BEGIN Header_angle_sensor_f */
/**
* @brief Function implementing the Angle_sensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_angle_sensor_f */
void angle_sensor_f(void const * argument)
{
  /* USER CODE BEGIN angle_sensor_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END angle_sensor_f */
}

/* USER CODE BEGIN Header_motor_run_f */
/**
* @brief Function implementing the Motor_run thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_motor_run_f */
void motor_run_f(void const * argument)
{
  /* USER CODE BEGIN motor_run_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END motor_run_f */
}

/* USER CODE BEGIN Header_motor_sensor_f */
/**
* @brief Function implementing the Motor_sensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_motor_sensor_f */
void motor_sensor_f(void const * argument)
{
  /* USER CODE BEGIN motor_sensor_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END motor_sensor_f */
}

/* USER CODE BEGIN Header_balance_f */
/**
* @brief Function implementing the Balance thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_balance_f */
void balance_f(void const * argument)
{
  /* USER CODE BEGIN balance_f */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END balance_f */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
