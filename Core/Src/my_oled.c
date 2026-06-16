/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * 文件名称          : my_oled.c
 * 功能描述          : OLED 主动获取数据显示（16像素大字，4行）
 *                    通过注释 DISPLAY_PID 宏切换 PID / 传感器模式
 *                    PID 模式同时显示内环+外环的 Kp/Ki/Kd（不再需要切换）
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
        /* ===== PID 参数显示模式（内环+外环同屏） ===== */
        extern PID_t AnglePID;
        extern PID_t LocationPID;

        static float last_inner_kp = 0, last_inner_ki = 0, last_inner_kd = 0;
        static float last_outer_kp = 0, last_outer_ki = 0, last_outer_kd = 0;
        uint8_t need_refresh = 1;
        char line_buf[32];

        for (;;) {
            /* 每 500ms 检查一次两个环的 PID 参数变化 */
            static uint32_t last_check = 0;
            if (HAL_GetTick() - last_check > 500) {
                last_check = HAL_GetTick();

                if (AnglePID.Kp != last_inner_kp || AnglePID.Ki != last_inner_ki || AnglePID.Kd != last_inner_kd ||
                    LocationPID.Kp != last_outer_kp || LocationPID.Ki != last_outer_ki || LocationPID.Kd != last_outer_kd) {
                    last_inner_kp = AnglePID.Kp;
                    last_inner_ki = AnglePID.Ki;
                    last_inner_kd = AnglePID.Kd;
                    last_outer_kp = LocationPID.Kp;
                    last_outer_ki = LocationPID.Ki;
                    last_outer_kd = LocationPID.Kd;
                    need_refresh = 1;
                }
            }

            if (need_refresh) {
                /* 清空 4 行 */
                for (uint8_t y = 0; y < 4; y++)
                    OLED_ShowStr(0, y * 2, "               ", FONT_SIZE);

                /* 第1行：标题 */
                snprintf(line_buf, sizeof(line_buf), "PID:Inner/Outer");
                OLED_ShowStr(0, 0, line_buf, FONT_SIZE);

                /* 第2行：Kp（内环 | 外环） */
                snprintf(line_buf, sizeof(line_buf), "Kp:%.2f|%.3f",
                         (double)AnglePID.Kp, (double)LocationPID.Kp);
                OLED_ShowStr(0, 2, line_buf, FONT_SIZE);

                /* 第3行：Ki */
                snprintf(line_buf, sizeof(line_buf), "Ki:%.2f|%.2f",
                         (double)AnglePID.Ki, (double)LocationPID.Ki);
                OLED_ShowStr(0, 4, line_buf, FONT_SIZE);

                /* 第4行：Kd */
                snprintf(line_buf, sizeof(line_buf), "Kd:%.2f|%.2f",
                         (double)AnglePID.Kd, (double)LocationPID.Kd);
                OLED_ShowStr(0, 6, line_buf, FONT_SIZE);

                need_refresh = 0;
            }
            osDelay(REFRESH_MS);
        }

    #else
        /* ===== 传感器数据显示模式（100ms 检测，无闪烁） ===== */
        Sensor_Data_Typedef sensor;
        MotorFeedback_t motor_fb;
        uint8_t need_refresh = 1;
        char line_buf[32];

        for (;;) {
            static uint32_t last_update = 0;
            if (HAL_GetTick() - last_update > 100) {
                last_update = HAL_GetTick();

                if (xQueuePeek(xSensorQueue, &sensor, 0) == pdTRUE) {
                    need_refresh = 1;
                }
                if (xQueuePeek(xMotorFeedbackQueue, &motor_fb, 0) == pdTRUE) {
                    need_refresh = 1;
                }
            }

            if (need_refresh) {
                snprintf(line_buf, sizeof(line_buf), "A1:%-10.1f", (double)sensor.angle1);
                OLED_ShowStr(0, 0, line_buf, FONT_SIZE);

                snprintf(line_buf, sizeof(line_buf), "A2:%-10.1f", (double)sensor.angle2);
                OLED_ShowStr(0, 2, line_buf, FONT_SIZE);

                snprintf(line_buf, sizeof(line_buf), "Spd:%-9.0f", (double)motor_fb.speed_rpm);
                OLED_ShowStr(0, 4, line_buf, FONT_SIZE);

                extern volatile uint8_t RunState;
                snprintf(line_buf, sizeof(line_buf), "Run:%-8u", RunState);
                OLED_ShowStr(0, 6, line_buf, FONT_SIZE);

                need_refresh = 0;
            }
            osDelay(REFRESH_MS);
        }
    #endif
}