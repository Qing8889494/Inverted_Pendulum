/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : angle_sensor.c
  * 功能描述          : ADC角度传感器数据采集，读取角度和角速度
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：周南全
  * 创建时间：2026年5月23日
  * 修改记录：把传感器的采集时间改成了5ms （周南全 2026.6.1）
  ******************************************************************************
  */
/* USER CODE END Header */

#include "angle_sensor.h"
#include "cmsis_os.h"

QueueHandle_t xSensorQueue = NULL;

// 一阶滤波
static float sensor_filter(float new_val, float old_val)
{
    return old_val * 0.9f + new_val * 0.1f;
}

// 读取传感器数据
void ADC_Sensor_Read(Sensor_Data_Typedef *data)
{
    static float last_adc1 = 0, last_adc2 = 0;
    static float last_angle1 = 0, last_angle2 = 0;  // 存储上一次角度

    // 读取原始ADC
    float raw1 = (float)adc_raw[0];
    float raw2 = (float)adc_raw[1];

    // 滤波
    float filter_adc1 = sensor_filter(raw1, last_adc1);
    float filter_adc2 = sensor_filter(raw2, last_adc2);

    // 计算校准角度（垂直=0°）
    data->angle1 = ((filter_adc1 - A1_VERTICAL_MID) / ADC_MAX) * FULL_ANGLE;
    data->angle2 = ((filter_adc2 - A2_VERTICAL_MID) / ADC_MAX) * FULL_ANGLE;

    // 计算角速度
    data->angular_velocity1 = (data->angle1 - last_angle1) / SAMPLE_PERIOD;
    data->angular_velocity2 = (data->angle2 - last_angle2) / SAMPLE_PERIOD;

    // 更新历史值
    last_adc1 = filter_adc1;
    last_adc2 = filter_adc2;
    last_angle1 = data->angle1;
    last_angle2 = data->angle2;
}

//任务函数定义
void angle_sensor_f(void const * argument)
{
  
  Sensor_Data_Typedef sensor_data;

    for(;;)
    {
        // 读取+滤波数据
        ADC_Sensor_Read(&sensor_data);

        // 发送给控制任务
        xQueueOverwrite(xSensorQueue, &sensor_data);

        // 5ms采集一次
        vTaskDelay(pdMS_TO_TICKS(5));
    }
 
}
