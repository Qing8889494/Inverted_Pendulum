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
/* USER CODE END Header */


#include "balance.h"
#include "cmsis_os.h"
#include "angle_sensor.h"
#include "motor.h"
#include "led.h"

#define CENTER_ANGLE		0		//这个是角度的中心位置
#define CENTER_RANGE		60		//这个是角度中心范围

// 完成调参之后进行使用
//#define START_PWM       600     // 起摆电机力度 (参考:35/100, 你这边范围0-1000, 对应约600)
//#define START_TIME      100     // 起摆瞬时驱动力持续时间(ms) (与参考一致)

void PID_Update(PID_t *p)
{
	/*获取本次误差和上次误差*/
	p->Error1 = p->Error0;					//获取上次误差
	p->Error0 = p->Target - p->Actual;		//获取本次误差，目标值减实际值，即为误差值
	
	/*外环误差积分（累加）*/
	/*如果Ki不为0，才进行误差积分，这样做的目的是便于调试*/
	/*因为在调试时，我们可能先把Ki设置为0，这时积分项无作用，误差消除不了，误差积分会积累到很大的值*/
	/*后续一旦Ki不为0，那么因为误差积分已经积累到很大的值了，这就导致积分项疯狂输出，不利于调试*/
	if (p->Ki > 0.001f || p->Ki < -0.001f)					//如果Ki不为0
	{
		p->ErrorInt += p->Error0;	//进行误差积分
	}
	else							//否则
	{
		p->ErrorInt = 0;			//误差积分直接归0
	}
	
	/*PID计算*/
	/*使用位置式PID公式，计算得到输出值*/
	p->Out = p->Kp * p->Error0
		   + p->Ki * p->ErrorInt
		   + p->Kd * (p->Error0 - p->Error1);
	
	/*输出限幅*/
	if (p->Out > p->OutMax) {p->Out = p->OutMax;}	//限制输出值最大为结构体指定的OutMax
	if (p->Out < p->OutMin) {p->Out = p->OutMin;}	//限制输出值最小为结构体指定的OutMin
	
	if(p->Ki != 0) //防止除以零造成程序死机
	{
		if (p->ErrorInt > p->OutMax / p->Ki) p->ErrorInt = p->OutMax / p->Ki;
		if (p->ErrorInt < p->OutMin / p->Ki) p->ErrorInt = p->OutMin / p->Ki;
	}
}


PID_t AnglePID = {			//内环角度环PID结构体变量，定义的时候同时给部分成员赋初值
	.Target = CENTER_ANGLE,	//角度环目标值初值设定为中心角度值
	
	.Kp = 2.0f,				//比例项权重
	.Ki = 0.0f,				//积分项权重
	.Kd = 0.8f,				//微分项权重
	
	.OutMax = 1000,			//输出限幅的最大值
	.OutMin = -1000,			//输出限幅的最小值
};

PID_t LocationPID = {		//外环位置环PID结构体变量，定义的时候同时给部分成员赋初值
	.Target = 0,			//位置环目标值初值设定为0
	
	.Kp = 1.0f,				//比例项权重
	.Ki = 0.02f,				//积分项权重
	.Kd = 0.1f,				//微分项权重
	
	.OutMax = 60,			//输出限幅的最大值
	.OutMin = -60,			//输出限幅的最小值
};

Sensor_Data_Typedef angle;	//这里得到第一个角度的数据
MotorFeedback_t motor_sensor;//这里得到电机的数据
MotorCmd_t motor_command;	//这里是用于回传的

volatile uint8_t RunState = 1;			//表示倒立摆运行状态，0=停止, 1=判断状态, 21-24=向左泵, 31-34=向右泵, 4=平衡

//任务函数定义
void balance_f(void const * argument)
{
    static uint16_t Count1 = 0, Count2 = 0;
    Sensor_Data_Typedef angle_local;
    MotorFeedback_t motor_local;
    MotorCmd_t motor_cmd;

    for(;;)
    {
        // 阻塞等待最新传感器数据，15ms 超时
        if (xQueueReceive(xSensorQueue, &angle_local, pdMS_TO_TICKS(15)) == pdTRUE) {
            LED_Off(LED1_Pin);
        } else {
            LED_On(LED1_Pin);
            continue;   // 没拿到传感器数据，跳过本次控制
        }

        // 阻塞等待最新电机反馈
        if (xQueueReceive(xMotorFeedbackQueue, &motor_local, pdMS_TO_TICKS(15)) == pdTRUE) {
            LED_Off(LED2_Pin);
        } else {
            LED_On(LED2_Pin);
            continue;
        }

        // 摔倒保护
        if (angle_local.angle2 < CENTER_ANGLE - CENTER_RANGE ||
            angle_local.angle2 > CENTER_ANGLE + CENTER_RANGE) {
            RunState = 0;
            motor_cmd.target_speed = 0;
            xQueueSend(xMotorCmdQueue, &motor_cmd, 0);
							
							LED_On(LED2_Pin);
							
            continue;
        }

				LED_Off(LED2_Pin);
				
        // 控制分频（注意：每次循环只加一次，基于传感器更新频率）
        Count1++;
        Count2++;

        if (Count1 >= 5) {   // 每 5 个传感器数据更新一次角度环（约25ms)
            Count1 = 0;
            AnglePID.Actual = angle_local.angle2;
            PID_Update(&AnglePID);
            motor_cmd.target_speed = (int16_t)AnglePID.Out;
					
						if(motor_cmd.target_speed == 0) {
							LED_On(LED3_Pin);
						}else {
							LED_Off(LED3_Pin);
						}
            xQueueSend(xMotorCmdQueue, &motor_cmd, 0);
        }

        if (Count2 >= 25) {  // 每 25 个传感器数据更新一次位置环（约125ms）			
            Count2 = 0;
            LocationPID.Actual = motor_local.encoder_pos;
            PID_Update(&LocationPID);
            AnglePID.Target = CENTER_ANGLE - LocationPID.Out;
        }

        // 控制周期由队列接收超时决定（约15ms），无需额外 vTaskDelay
    }

}
//	这段是带有自动起摆的任务函数，在完成调参之后进行使用
//  void balance_f(void const * argument)
//  {
//      /* 起摆相关静态变量 */
//      static uint16_t CountJudge;             // 判断状态计次(40ms间隔)
//      static float Angle0, Angle1, Angle2;    // 本次、上次、上上次角度
//      static uint16_t CountTime;              // 起摆计时
//      static uint16_t Count1, Count2;         // PID分频计数
//      float norm_angle;

//      for(;;)
//      {
//          xQueueReceive(xSensorQueue, &angle, 0);
//          xQueueReceive(xMotorFeedbackQueue, &motor_sensor, 0);

//          /* 角度规一化到 [-180, 180]，0 = 竖直向上 */
//          norm_angle = angle.angle1;
//          while (norm_angle > 180.0f)  norm_angle -= 360.0f;
//          while (norm_angle < -180.0f) norm_angle += 360.0f;

//          /**************** 状态机调度 ****************/
//          if (RunState == 0)          // 停止
//          {
//              motor_command.target_speed = 0;
//              xQueueSend(xMotorCmdQueue, &motor_command, 0);
//          }
//          else if (RunState == 1)     // 判断状态：检测摆杆峰值点
//          {
//              CountJudge++;
//              if (CountJudge >= 40)   // 每40ms采样一次
//              {
//                  CountJudge = 0;

//                  /* 滑动更新3次角度值 */
//                  Angle2 = Angle1;
//                  Angle1 = Angle0;
//                  Angle0 = norm_angle;

//                  /* 判断是否在右侧最高点（即将向左摆）：中间值最小 → 向左推 */
//                  if (Angle0 > CENTER_RANGE
//                   && Angle1 > CENTER_RANGE
//                   && Angle2 > CENTER_RANGE
//                   && Angle1 < Angle0
//                   && Angle1 < Angle2)
//                  {
//                      RunState = 21;  // 进入向左泵序列
//                  }

//                  /* 判断是否在左侧最高点（即将向右摆）：中间值最大 → 向右推 */
//                  if (Angle0 < -CENTER_RANGE
//                   && Angle1 < -CENTER_RANGE
//                   && Angle2 < -CENTER_RANGE
//                   && Angle1 > Angle0
//                   && Angle1 > Angle2)
//                  {
//                      RunState = 31;  // 进入向右泵序列
//                  }

//                  /* 判断是否已进入平衡捕捉区间 */
//                  if (Angle0 > -SWITCH_ANGLE      // 自行定义 SWITCH_ANGLE 45
//                   && Angle0 < SWITCH_ANGLE
//                   && Angle1 > -SWITCH_ANGLE
//                   && Angle1 < SWITCH_ANGLE)
//                  {
//                      /* 变量归零 */
//                      LocationPID.ErrorInt = 0;
//                      AnglePID.ErrorInt = 0;
//                      AnglePID.Target = CENTER_ANGLE;

//                      RunState = 4;   // 切换到PID平衡
//                  }
//              }
//          }
//          else if (RunState == 21)    // 向左泵：电机左转
//          {
//              motor_command.target_speed = START_PWM;
//              xQueueSend(xMotorCmdQueue, &motor_command, 0);
//              CountTime = START_TIME;
//              RunState = 22;
//          }
//          else if (RunState == 22)    // 向左泵：持续计时
//          {
//              CountTime--;
//              if (CountTime == 0) RunState = 23;
//          }
//          else if (RunState == 23)    // 向左泵：电机右转
//          {
//              motor_command.target_speed = -START_PWM;
//              xQueueSend(xMotorCmdQueue, &motor_command, 0);
//              CountTime = START_TIME;
//              RunState = 24;
//          }
//          else if (RunState == 24)    // 向左泵：持续计时
//          {
//              CountTime--;
//              if (CountTime == 0)
//              {
//                  motor_command.target_speed = 0;
//                  xQueueSend(xMotorCmdQueue, &motor_command, 0);
//                  RunState = 1;       // 回到判断状态
//              }
//          }
//          else if (RunState == 31)    // 向右泵：电机右转
//          {
//              motor_command.target_speed = -START_PWM;
//              xQueueSend(xMotorCmdQueue, &motor_command, 0);
//              CountTime = START_TIME;
//              RunState = 32;
//          }
//          else if (RunState == 32)    // 向右泵：持续计时
//          {
//              CountTime--;
//              if (CountTime == 0) RunState = 33;
//          }
//          else if (RunState == 33)    // 向右泵：电机左转
//          {
//              motor_command.target_speed = START_PWM;
//              xQueueSend(xMotorCmdQueue, &motor_command, 0);
//              CountTime = START_TIME;
//              RunState = 34;
//          }
//          else if (RunState == 34)    // 向右泵：持续计时
//          {
//              CountTime--;
//              if (CountTime == 0)
//              {
//                  motor_command.target_speed = 0;
//                  xQueueSend(xMotorCmdQueue, &motor_command, 0);
//                  RunState = 1;       // 回到判断状态
//              }
//          }
//          else if (RunState == 4)     // PID平衡控制
//          {
//              /* 摔倒保护 */
//              if (!(norm_angle > -CENTER_RANGE && norm_angle < CENTER_RANGE))
//              {
//                  RunState = 0;       // 停止
//              }

//              /* 角度环 5ms */
//              Count1++;
//              if (Count1 >= 5)
//              {
//                  Count1 = 0;
//                  AnglePID.Actual = angle.angle1;
//                  PID_Update(&AnglePID);
//                  motor_command.target_speed = (int16_t)AnglePID.Out;
//                  xQueueSend(xMotorCmdQueue, &motor_command, 0);
//              }

//              /* 位置环 25ms */
//              Count2++;
//              if (Count2 >= 25)
//              {
//                  Count2 = 0;
//                  LocationPID.Actual = motor_sensor.encoder_pos;
//                  PID_Update(&LocationPID);
//                  AnglePID.Target = CENTER_ANGLE - LocationPID.Out;
//              }
//          }

//          vTaskDelay(pdMS_TO_TICKS(1));
//      }
//  }

void set_f(void const * argument)
{
	
	GPIO_PinState key_now;    // 当前按键电平
  static uint8_t key_last = 0; // 上次按键状态
  
	for(;;)
	{
		
		key_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8);
		/*控制启动*/
		if (key_last == GPIO_PIN_RESET && key_now == GPIO_PIN_SET)			//留出判断条件
		{
			osDelay(20);
			
			if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == GPIO_PIN_SET)
      {
        RunState = !RunState;  // 翻转启停状态：按一下开，再按一下关
      }
		}
		
		key_last = key_now;
		
		/*LED指示程序运行状态*/
		if (RunState)		//如果运行状态非0
		{
			LED_On(LED2_Pin);		//点亮LED，指示程序正在运行
		}
		else				//否则
		{
			LED_Off(LED2_Pin);		//熄灭LED，指示程序停止运行
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

