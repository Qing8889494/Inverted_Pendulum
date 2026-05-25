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
	*						用串口2的收数据，来接收电脑端发送的电机速度指令：@v 300
	*						增加串口2 SerialPlot 波形发送（向思嘉，2026.5.25）
  *           增加串口3 PID 参数接收（向思嘉，2026.5.25）
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


// 引用 balance.c 中的 PID 全局变量（用于更新系数）
typedef struct {
    float Target;
    float Actual;
    float Kp, Ki, Kd;
    float Error0, Error1, ErrorInt;
    float Out, OutMax, OutMin;
} PID_t;

extern PID_t AnglePID;      // 内环角度环
extern PID_t LocationPID;   // 外环位置环

// PID 参数结构体（用于队列）
typedef struct {
    float inner_kp;
    float inner_ki;
    float inner_kd;
    float outer_kp;
    float outer_ki;
    float outer_kd;
} PID_Params_t;

// 队列句柄
static QueueHandle_t xPIDQueue = NULL;

//任务函数定义
/* ========== 串口2 任务：发送波形数据给 SerialPlot ========== */
void usart2_tx_f(void const * argument)
{
	Sensor_Data_Typedef sensor_data;
    MotorCmd_t motor_cmd;
    float plot_data[3];
    char tx_buf[128];
    char *ptr;
    uint8_t i;

    for(;;)
    {
        // 获取传感器数据（角度、角速度）
        if (xQueuePeek(xSensorQueue, &sensor_data, 0) != pdPASS)
        {
            osDelay(10);
            continue;
        }
        // 获取电机当前目标速度
        if (xQueuePeek(xMotorCmdQueue, &motor_cmd, 0) != pdPASS)
        {
            motor_cmd.target_speed = 0;  // 默认值
        }

        // 准备三个通道数据：角度1、角速度1、电机目标速度
        plot_data[0] = sensor_data.angle1;
        plot_data[1] = sensor_data.angular_velocity1;
        plot_data[2] = (float)motor_cmd.target_speed;

        // 格式化字符串：数值1,数值2,数值3\r\n
        ptr = tx_buf;
        for (i = 0; i < 3; i++)
        {
            ptr += sprintf(ptr, "%.3f", plot_data[i]);
            if (i < 2) *ptr++ = ',';
        }
        *ptr++ = '\r';
        *ptr++ = '\n';
        *ptr = '\0';
				
				// 通过串口2发送
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, ptr - tx_buf, 100);

        // 发送间隔 20ms（可根据需要调整）
        osDelay(20);
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

/* ========== 串口3 任务：接收 PID 参数并更新 ========== */
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
    uint8_t      rx_char;
    uint8_t      rx_buf[64];
    uint8_t      idx = 0;
    PID_Params_t pid;

    for(;;)
    {
        if (HAL_UART_Receive(&huart3, &rx_char, 1, 100) == HAL_OK)
        {
            if (rx_char == '\n')
            {
                rx_buf[idx] = '\0';

                if (idx > 0 && xPIDQueue != NULL)
                {
                    if (sscanf((char*)rx_buf, "%f,%f,%f,%f,%f,%f",
                               &pid.inner_kp, &pid.inner_ki, &pid.inner_kd,
                               &pid.outer_kp, &pid.outer_ki, &pid.outer_kd) == 6)
                    {
                        // 覆盖写入：保证队列中始终是最新一帧
                        xQueueOverwrite(xPIDQueue, &pid);
                    }
                    // 解析失败静默丢弃，不影响控制任务
                }

                idx = 0;
            }
            else if (rx_char != '\r')
            {
                if (idx < sizeof(rx_buf) - 1)
                    rx_buf[idx++] = rx_char;
                else
                    idx = 0;  // 缓冲区溢出，丢弃当前帧
            }
        }

        osDelay(1);
    }
}


static void PIDUpdateTask(void *pvParameters)
{
    PID_Params_t new_pid;

    for(;;)
    {
        // 阻塞等待队列中有新数据（超时100ms）
        if (xQueueReceive(xPIDQueue, &new_pid, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // 用 FreeRTOS 临界区保护，防止控制任务读到写到一半的参数
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

            // 通过串口1发送确认（注意：与串口1其他任务存在竞争，
            // 仅影响调试打印，不影响控制逻辑，可按需删除此行）
            char ack[] = "PID updated via queue\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t*)ack, strlen(ack), 100);
        }
    }
}

/* ==========================================================================
 * USART_Init — 统一初始化函数
 *
 * 在 main.c 中，将原来的：
 *   USART3_InitQueue();
 * 替换为：
 *   USART_Init();
 * 必须在 osKernelStart() 之前调用。
 * ========================================================================== */
void USART_Init(void)
{
    // 创建 PID 队列（长度1，覆盖模式）
    xPIDQueue = xQueueCreate(1, sizeof(PID_Params_t));
    configASSERT(xPIDQueue != NULL);

    // 创建 PID 更新任务（优先级2，低于平衡控制任务）
    BaseType_t ret = xTaskCreate(PIDUpdateTask, "PIDUpd", 256, NULL, 2, NULL);
    configASSERT(ret == pdPASS);
}
