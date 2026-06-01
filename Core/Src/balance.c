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
	
	.Kp = 0,				//比例项权重
	.Ki = 0,				//积分项权重
	.Kd = 0,				//微分项权重
	
	.OutMax = 1000,			//输出限幅的最大值
	.OutMin = -1000,			//输出限幅的最小值
};

PID_t LocationPID = {		//外环位置环PID结构体变量，定义的时候同时给部分成员赋初值
	.Target = 0,			//位置环目标值初值设定为0
	
	.Kp = 0,				//比例项权重
	.Ki = 0,				//积分项权重
	.Kd = 0,				//微分项权重
	
	.OutMax = 60,			//输出限幅的最大值
	.OutMin = -60,			//输出限幅的最小值
};

Sensor_Data_Typedef angle;	//这里得到第一个角度的数据
MotorFeedback_t motor_sensor;//这里得到电机的数据
MotorCmd_t motor_command;	//这里是用于回传的

volatile uint8_t RunState;			//表示倒立摆运行状态

//任务函数定义
void balance_f(void const * argument)
{
	
	static uint16_t Count1, Count2;
	
	for(;;)
	{	
		
		xQueueReceive(xSensorQueue, &angle, 0);
		xQueueReceive(xMotorFeedbackQueue, &motor_sensor, 0);
		
		/*摆杆倒下自动停止PID程序*/
		if (! (angle.angle1 > CENTER_ANGLE - CENTER_RANGE
			&& angle.angle1 < CENTER_ANGLE + CENTER_RANGE))	//如果角度值超过了规定的中心区间
		{
			RunState = 0;			//运行状态变量置0，自动停止PID程序
		}
		
				/*根据运行状态执行PID程序或者停止*/
		if (RunState)				//如果运行状态不为0
		{
			/*角度环计次分频*/
			Count1 ++;				//计次自增
			if (Count1 >= 5)		//如果计次5次，则if成立，即if每隔5ms进一次
			{
				Count1 = 0;			//计次清零，便于下次计次
				
				/*以下进行角度环PID控制*/
				AnglePID.Actual = angle.angle1;		//内环为角度环，实际值为角度值
				PID_Update(&AnglePID);					//调用封装好的函数，一步完成PID计算和更新
				motor_command.target_speed = AnglePID.Out;
				xQueueSend(xMotorCmdQueue, &motor_command, 0);//角度环的输出值给到电机	
			}
			
			/*位置环计次分频*/
			Count2 ++;				//计次自增
			if (Count2 >= 25)		//如果计次50次，则if成立，即if每隔50ms进一次
			{
				Count2 = 0;			//计次清零，便于下次计次
				
				/*以下进行位置环PID控制*/
				
				LocationPID.Actual = motor_sensor.encoder_pos;	//外环为位置环，实际值为位置值
				PID_Update(&LocationPID);		//调用封装好的函数，一步完成PID计算和更新
				AnglePID.Target = CENTER_ANGLE - LocationPID.Out;	//外环的输出值作用于内环的目标值，组成串级PID结构
			}
		}
		else						//如果运行状态为0
		{
			motor_command.target_speed = 0;
			xQueueSend(xMotorCmdQueue, &motor_command, 0);		//不执行PID程序且电机PWM直接设置为0，电机停止
		}
		
		vTaskDelay(pdMS_TO_TICKS(1));
	}
  
}

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
			LED_On(LED1_Pin);		//点亮LED，指示程序正在运行
		}
		else				//否则
		{
			LED_Off(LED1_Pin);		//熄灭LED，指示程序停止运行
		}
	  
		osDelay(1);
	}
  
}

