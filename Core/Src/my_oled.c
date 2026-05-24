/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_oled.c
  * 功能描述          : 想在OLED屏上展示数据,只需要调用函数oled_show(const char *label, float value),label为你的数据名称,value就是你的数据
			记得加上头文件#iinclude "my_oled.h"
  ******************************************************************************
  * @attention
  *
  * 版权所有：控制工程课程项目组
  * 保留所有权利
  *
  * 作    者：杨洲
  * 创建时间：2026.5.23
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

/* ========== 全局变量（仅在此定义） ========== */
#define OLED_CMD_BUF_SIZE   32
volatile char   oled_cmd_str[OLED_CMD_BUF_SIZE];
volatile uint8_t oled_cmd_new = 0;

/* ========== 发送接口 ========== */
void oled_show(const char *label, float value)
{
    snprintf((char *)oled_cmd_str, OLED_CMD_BUF_SIZE, "%s:%.2f", label, value);
    oled_cmd_new = 1;
}

/* ========== OLED 显示任务 ========== */
void oled_f(void const * argument)
{
    /* ---------- 可调参数 ---------- */
    #define MAX_DISPLAY_LINES   6
    #define STR_BUF_SIZE        32
    #define REFRESH_MS          30
    #define FONT_SIZE           16
    #define CHAR_WIDTH          (FONT_SIZE / 2)

    /* ---------- 数据结构 ---------- */
    typedef struct {
        char    label[16];
        float   value;
    } CmdData_t;

    typedef struct {
        char    label[16];
        float   value;
        uint8_t active;
    } DisplayItem_t;

    static DisplayItem_t items[MAX_DISPLAY_LINES] = {0};
    static uint8_t       item_count = 0;
    static uint8_t       first_run  = 1;

    char local_cmd[OLED_CMD_BUF_SIZE];
    char line_buf[STR_BUF_SIZE];
    uint8_t y;

    /* ---------- 一次性软件初始化 ---------- */
    if (first_run) {
        // 注意：严禁再调用 MX_I2C1_Init()！I2C硬件已在main中初始化完毕。
        OLED_Init();        // 发送命令初始化OLED控制器（使用已就绪的I2C）
        OLED_Clear();       // 清屏
        first_run = 0;
    }

    /* ---------- 主循环 ---------- */
    for (;;) {
        /* 1. 处理新命令 */
        if (oled_cmd_new) {
            strncpy(local_cmd, (const char *)oled_cmd_str, OLED_CMD_BUF_SIZE - 1);
            local_cmd[OLED_CMD_BUF_SIZE - 1] = '\0';
            oled_cmd_new = 0;

            CmdData_t cmd;
            if (sscanf(local_cmd, "%[^:]:%f", cmd.label, &cmd.value) == 2) {
                int8_t idx = -1;
                for (uint8_t i = 0; i < item_count; i++) {
                    if (strcasecmp(items[i].label, cmd.label) == 0) {
                        idx = i;
                        break;
                    }
                }
                if (idx >= 0) {
                    items[idx].value = cmd.value;          // 更新已有项
                } else {
                    if (item_count < MAX_DISPLAY_LINES) {  // 插入新项
                        idx = item_count++;
                        strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
                        items[idx].value = cmd.value;
                        items[idx].active = 1;
                        // 标签首字母大写
                        if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z')
                            items[idx].label[0] -= ('a' - 'A');
                    } else {                               // 表满，移除最早项
                        for (uint8_t i = 0; i < MAX_DISPLAY_LINES - 1; i++)
                            items[i] = items[i + 1];
                        idx = MAX_DISPLAY_LINES - 1;
                        strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
                        items[idx].value = cmd.value;
                        if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z')
                            items[idx].label[0] -= ('a' - 'A');
                    }
                }
            }
        }

        /* 2. 刷新屏幕 */
        y = 0;
        for (uint8_t i = 0; i < item_count && y < MAX_DISPLAY_LINES; i++) {
            snprintf(line_buf, STR_BUF_SIZE, "%s:", items[i].label);
            OLED_ShowStr(0, y, line_buf, FONT_SIZE);

            int x_val = strlen(items[i].label) * CHAR_WIDTH + CHAR_WIDTH; // 冒号宽度
            snprintf(line_buf, STR_BUF_SIZE, "%.2f", (double)items[i].value);
            OLED_ShowStr(x_val, y, line_buf, FONT_SIZE);
            y++;
        }
        for (; y < MAX_DISPLAY_LINES; y++) {
            OLED_ShowStr(0, y, "                    ", FONT_SIZE);
        }

        osDelay(REFRESH_MS);
    }
}