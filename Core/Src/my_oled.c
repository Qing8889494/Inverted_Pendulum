/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_oled.c
  * 功能描述          : 想在OLED屏上展示数据,只需要调用函数oled_show(const char *label, float value),label为你的数据名称,value就是你的数据
			记得加上头文件#iinclude "my_oled.h"
  ******************************************************************************
  * @attention
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
    // 复位强制清空残留数据（解决多点/乱码）
    oled_cmd_new = 0;
    memset((void*)oled_cmd_str, 0, OLED_CMD_BUF_SIZE);

    /* ---------- 核心修复：0.91寸OLED只有4行！ ---------- */
    #define MAX_DISPLAY_LINES   4    // 0.91寸屏固定4行，原来的6行是BUG！
    #define STR_BUF_SIZE        32
    #define REFRESH_MS          30
    #define FONT_SIZE           16
    #define CHAR_WIDTH          8

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

    char local_cmd[OLED_CMD_BUF_SIZE];
    char line_buf[STR_BUF_SIZE];
    uint8_t y;

    /* ---------- 每次复位都强制初始化 ---------- */
	OLED_Init();
	HAL_Delay(50);          // 等待屏幕硬件就绪
	OLED_Clear();
	HAL_Delay(30);
	OLED_Clear();           // 二次清屏杜绝残留
	memset(items, 0, sizeof(items));
	item_count = 0;

	// 启动画面
	OLED_ShowStr(0, 0, "System Boot", 16);
	OLED_ShowStr(0, 1, "Success!", 16);
	osDelay(800);
	OLED_Clear();
	HAL_Delay(30);

    /* ---------- 主循环 ---------- */
    for (;;)
    {
        /* 1. 处理新命令 */
        if (oled_cmd_new)
        {
            strncpy(local_cmd, (const char *)oled_cmd_str, OLED_CMD_BUF_SIZE - 1);
            local_cmd[OLED_CMD_BUF_SIZE - 1] = '\0';
            oled_cmd_new = 0;

            CmdData_t cmd;
            if (sscanf(local_cmd, "%[^:]:%f", cmd.label, &cmd.value) == 2)
            {
                int8_t idx = -1;
                for (uint8_t i = 0; i < item_count; i++)
                {
                    if (strcasecmp(items[i].label, cmd.label) == 0)
                    {
                        idx = i;
                        break;
                    }
                }
                if (idx >= 0)
                {
                    items[idx].value = cmd.value;
                }
                else
                {
                    if (item_count < MAX_DISPLAY_LINES)
                    {
                        idx = item_count++;
                        strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
                        items[idx].value = cmd.value;
                        items[idx].active = 1;
                        if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z')
                            items[idx].label[0] -= 32;
                    }
                }
            }
        }

        /* 2. 刷新屏幕：先清空全部，再显示（无残留） */
		for(y=0; y<MAX_DISPLAY_LINES; y++)
		{
			OLED_ShowStr(0, y, "                ", FONT_SIZE);
		}

		// 显示数据
		y = 0;
		for (uint8_t i = 0; i < item_count; i++)
		{
			snprintf(line_buf, STR_BUF_SIZE, "%s:", items[i].label);
			OLED_ShowStr(0, y, line_buf, FONT_SIZE);
			// 数值固定放在48像素位置，不会超出屏幕
			snprintf(line_buf, STR_BUF_SIZE, "%.2f", (double)items[i].value);
			OLED_ShowStr(48, y, line_buf, FONT_SIZE);
			y++;
		}

        osDelay(REFRESH_MS);
    }
}
