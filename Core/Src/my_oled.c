/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_oled.c
  * 功能描述          : 
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：杨洲
  * 创建时间：
  * 使用说明：OLED 主动获取数据显示（16像素大字，4行，自动翻页每 500ms 从传感器队列、电机反馈队列、
			  PID 参数及 RunState 拉取数据。不依赖 oled_show 推送，但保留该函数以兼容可能存在的旧调用
			  通过注释宏切换显示模式
  * 修改记录：
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
#include "my_oled.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "oled.h"
/* 数据来源头文件 */
#include "angle_sensor.h"   // 传感器队列 xSensorQueue
#include "motor.h"          // 电机反馈队列 xMotorFeedbackQueue
#include "balance.h"        // PID 全局变量 AnglePID, LocationPID
/* ========== 显示模式切换 ========== */
#define DISPLAY_PID       /* 注释此行则切换为传感器数据显示 */
//#define SHOW_INNER_PID   /* PID 模式下：注释则显示外环，取消注释显示内环 */
/* ========== 全局变量（保留，兼容其他文件对 oled_show 的调用） ========== */
#define OLED_CMD_BUF_SIZE   32
volatile char   oled_cmd_str[OLED_CMD_BUF_SIZE];
volatile uint8_t oled_cmd_new = 0;
void oled_show(const char *label, float value)
{
    snprintf((char *)oled_cmd_str, OLED_CMD_BUF_SIZE, "%s:%.2f", label, value);
    oled_cmd_new = 1;
}
/* ========== OLED 显示任务 ========== */
void oled_f(void const * argument)
{
    #define FONT_SIZE          16
    #define REFRESH_MS         30
    #define MAX_CHARS_LINE     15
    /* ---------- 启动画面 ---------- */
    static uint8_t first_run = 1;
    if (first_run) {
        OLED_Init();
        HAL_Delay(50);
        OLED_Clear();
        HAL_Delay(20);
        OLED_Clear();
        OLED_ShowStr(0, 0, "  System Boot", FONT_SIZE);
        OLED_ShowStr(0, 2, "  Success!", FONT_SIZE);
        osDelay(1500);
        OLED_Clear();
        first_run = 0;
    }
    /* ---------- 根据宏选择显示内容 ---------- */
    #ifdef DISPLAY_PID
        /* ===== PID 参数显示模式 ===== */
        #ifdef SHOW_INNER_PID
            const char *title = "Inner PID";
            #define PID_STRUCT  AnglePID
        #else
            const char *title = "Outer PID";
            #define PID_STRUCT  LocationPID
        #endif
        extern PID_t AnglePID;
        extern PID_t LocationPID;
        static float last_kp = 0.0f, last_ki = 0.0f, last_kd = 0.0f;
        uint8_t need_refresh = 1;
        char line_buf[32];
        for (;;) {
            /* 每 500ms 检查一次 PID 参数变化 */
            static uint32_t last_check = 0;
            if (HAL_GetTick() - last_check > 500) {
                last_check = HAL_GetTick();
                float kp = PID_STRUCT.Kp;
                float ki = PID_STRUCT.Ki;
                float kd = PID_STRUCT.Kd;
                if (kp != last_kp || ki != last_ki || kd != last_kd) {
                    last_kp = kp; last_ki = ki; last_kd = kd;
                    need_refresh = 1;
                }
            }
            if (need_refresh) {
                /* 清空 4 行 */
                for (uint8_t y = 0; y < 4; y++)
                    OLED_ShowStr(0, y * 2, "               ", FONT_SIZE);
                /* 标题 */
                snprintf(line_buf, sizeof(line_buf), "%s", title);
                OLED_ShowStr(0, 0, line_buf, FONT_SIZE);
                /* Kp */
                snprintf(line_buf, sizeof(line_buf), "Kp:%.2f", (double)PID_STRUCT.Kp);
                OLED_ShowStr(0, 2, line_buf, FONT_SIZE);
                /* Ki */
                snprintf(line_buf, sizeof(line_buf), "Ki:%.3f", (double)PID_STRUCT.Ki);
                OLED_ShowStr(0, 4, line_buf, FONT_SIZE);
                /* Kd */
                snprintf(line_buf, sizeof(line_buf), "Kd:%.3f", (double)PID_STRUCT.Kd);
                OLED_ShowStr(0, 6, line_buf, FONT_SIZE);
                need_refresh = 0;
            }
            osDelay(REFRESH_MS);
        }
    #else
        /* ===== 传感器数据显示模式 ===== */
        /* 可根据需要修改显示的标签和变量 */
        Sensor_Data_Typedef sensor;
        MotorFeedback_t motor_fb;
        static float last_angle1 = 0.0f, last_angle2 = 0.0f;
        static float last_speed = 0.0f;
        uint8_t need_refresh = 1;
        char line_buf[32];
        for (;;) {
            /* 每 200ms 更新一次传感器数据 */
            static uint32_t last_update = 0;
            if (HAL_GetTick() - last_update > 200) {
                last_update = HAL_GetTick();
                /* 读取角度（队列 Peek 不删除） */
                if (xQueuePeek(xSensorQueue, &sensor, 0) == pdTRUE) {
                    if (sensor.angle1 != last_angle1 || sensor.angle2 != last_angle2) {
                        last_angle1 = sensor.angle1;
                        last_angle2 = sensor.angle2;
                        need_refresh = 1;
                    }
                }
                /* 读取电机转速 */
                if (xQueuePeek(xMotorFeedbackQueue, &motor_fb, 0) == pdTRUE) {
                    if (motor_fb.speed_rpm != last_speed) {
                        last_speed = motor_fb.speed_rpm;
                        need_refresh = 1;
                    }
                }
            }
            if (need_refresh) {
                /* 清空 4 行 */
                for (uint8_t y = 0; y < 4; y++)
                    OLED_ShowStr(0, y * 2, "               ", FONT_SIZE);
                /* 角度1 */
                snprintf(line_buf, sizeof(line_buf), "A1:%.1f", (double)last_angle1);
                OLED_ShowStr(0, 0, line_buf, FONT_SIZE);
                /* 角度2 */
                snprintf(line_buf, sizeof(line_buf), "A2:%.1f", (double)last_angle2);
                OLED_ShowStr(0, 2, line_buf, FONT_SIZE);
                /* 电机转速 */
                snprintf(line_buf, sizeof(line_buf), "Spd:%.0f", (double)last_speed);
                OLED_ShowStr(0, 4, line_buf, FONT_SIZE);
                /* 第四行可显示其他参数（如运行状态） */
                extern volatile uint8_t RunState;
                snprintf(line_buf, sizeof(line_buf), "Run:%u", RunState);
                OLED_ShowStr(0, 6, line_buf, FONT_SIZE);
                need_refresh = 0;
            }
            osDelay(REFRESH_MS);
        }
    #endif
}
