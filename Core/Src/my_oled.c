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


extern uint16_t adc_raw[ADC_CHANNEL_NUM];
/* OLED 显示任务用到的字符串接口 */
#define OLED_CMD_BUF_SIZE   32          		// 命令缓冲区大小
volatile char   oled_cmd_str[OLED_CMD_BUF_SIZE];   	// 存放 "angle:45.6" 等命令
volatile uint8_t oled_cmd_new = 0;     

void oled_show(const char *label, float value)
{
    snprintf((char *)oled_cmd_str, 32, "%s:%.2f", label, value);
    oled_cmd_new = 1;
}

//任务函数定义
void oled_f(void const * argument)
{
	/* ========== 内部可调参数 ========== */
	#define MAX_DISPLAY_LINES   6           /* 屏幕最多显示的行数 */
	#define STR_BUF_SIZE        32          /* 每行格式化缓冲区大小 */
	#define REFRESH_MS          30          /* 刷新周期（毫秒） */
	#define FONT_SIZE           16          /* 字体高度 */
	#define CHAR_WIDTH          (FONT_SIZE / 2)  /* 字符宽度（等宽字体估算） */

	/* ========== 命令字符串解析结构 ========== */
	typedef struct {
		char    label[16];      /* 命令标签，如 "angle" */
		float   value;          /* 数值 */
	} CmdData_t;

	/* ========== 显示的参数项 ========== */
	typedef struct {
		char    label[16];      /* 屏幕显示的标签，如 "Angle"（首字母大写） */
		float   value;          /* 当前值 */
		uint8_t active;         /* 此项是否有效 */
	} DisplayItem_t;

	static DisplayItem_t items[MAX_DISPLAY_LINES] = {0};
	static uint8_t       item_count = 0;
	static uint8_t       first_run  = 1;

	/* 缓冲区 */
	char local_cmd[OLED_CMD_BUF_SIZE];      // 复制全局命令，避免被覆盖
	char line_buf[STR_BUF_SIZE];
	uint8_t y;

	OLED_Init();         // ← 通过I2C发送初始化命令给OLED控制器
	/* 首次运行：清屏 */
	if (first_run) {
		MX_I2C1_Init();
		OLED_Init();   
		OLED_Clear();  
		first_run = 0;
	}

	for (;;)
	{
		/* ---- 1. 检查是否有新命令 ---- */
		if (oled_cmd_new) {
		/* 原子性复制全局字符串到本地（简单方式：直接拷贝） */
		strncpy(local_cmd, (const char *)oled_cmd_str, OLED_CMD_BUF_SIZE - 1);
		local_cmd[OLED_CMD_BUF_SIZE - 1] = '\0';
		oled_cmd_new = 0;   // 清除标志，表示已读取

		/* 解析字符串：格式 "label:value"，例如 "angle:45.6" */
		CmdData_t cmd;
		if (sscanf(local_cmd, "%[^:]:%f", cmd.label, &cmd.value) == 2)
		{
			/* 查找该标签是否已在显示项中 */
			int8_t idx = -1;
			for (uint8_t i = 0; i < item_count; i++) {
			if (strcasecmp(items[i].label, cmd.label) == 0) {   // 忽略大小写比较
				idx = i;
				break;
			}
                }

                if (idx >= 0) {
			/* 已存在：更新数值 */
			items[idx].value = cmd.value;
                } else {
			/* 新标签：插入显示表 */
			if (item_count < MAX_DISPLAY_LINES) {
				idx = item_count++;
				strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
				items[idx].value = cmd.value;
				items[idx].active = 1;
			/* 将首字母大写，使屏幕显示更美观 */
			if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z') {
				items[idx].label[0] -= ('a' - 'A');
				}
			} else {
				/* 表已满：移除最早项（所有项前移） */
				for (uint8_t i = 0; i < MAX_DISPLAY_LINES - 1; i++) {
				items[i] = items[i + 1];
				}
			idx = MAX_DISPLAY_LINES - 1;
			strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
			items[idx].value = cmd.value;
                        if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z') {
                            items[idx].label[0] -= ('a' - 'A');
                        }
                    }
                }
            }
            /* 若解析失败（格式不对），丢弃命令，不更新屏幕 */
        }

		/* ---- 2. 刷新 OLED 显示 ---- */
		y = 0;
		for (uint8_t i = 0; i < item_count && y < MAX_DISPLAY_LINES; i++) {
		/* 每行显示： "Label: Value" */
		snprintf(line_buf, STR_BUF_SIZE, "%s:", items[i].label);
		OLED_ShowStr(0, y, line_buf, FONT_SIZE);

		/* 在同一行紧接着显示数值（根据标签长度计算偏移） */
		int x_val = strlen(items[i].label) * CHAR_WIDTH + CHAR_WIDTH; // +冒号宽度
		snprintf(line_buf, STR_BUF_SIZE, "%.2f", (double)items[i].value);
		OLED_ShowStr(x_val, y, line_buf, FONT_SIZE);
		y++;
        }

		/* 擦除未使用的行（避免残留） */
		for (; y < MAX_DISPLAY_LINES; y++) {
		OLED_ShowStr(0, y, "                    ", FONT_SIZE);
		}

		/* 延时，让出 CPU 同时控制刷新率 */
		osDelay(1);
	}
}
