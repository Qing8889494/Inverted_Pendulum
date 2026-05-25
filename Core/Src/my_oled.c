/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * 文件名称          : my_oled.c
  * 功能描述          : 调用 oled_show("标签", 数值) 即可在OLED上显示16像素大字
  *                    屏幕固定显示4行，超过4个参数会自动翻页（3秒一页）
  *                    需搭配0.96寸128×64 I2C OLED驱动 (oled.c已改为64行模式)
  *                    已优化闪烁：仅在数据变化或翻页时刷新屏幕
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

/* ========== OLED 显示任务 (16像素大字 + 自动翻页 + 无闪烁) ========== */
void oled_f(void const * argument)
{
    /* ---------- 可调参数 ---------- */
    #define MAX_VISIBLE_ROWS   4        // 一屏显示 4 行大字
    #define MAX_TOTAL_ITEMS    12       // 最多保存 12 个参数
    #define STR_BUF_SIZE       32
    #define REFRESH_MS         30
    #define FONT_SIZE          16       // 16 像素大字体
    #define MAX_CHARS_LINE     15       // 每行最多 15 个字符，防止越界

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

    static DisplayItem_t items[MAX_TOTAL_ITEMS] = {0};
    static uint8_t       item_count = 0;        // 实际参数个数
    static uint8_t       first_run  = 1;
    static uint8_t       scroll_offset = 0;     // 当前显示的起始索引
    static uint32_t      last_scroll_tick = 0;
    static uint8_t       need_refresh = 1;      // 需要刷新标志

    char local_cmd[OLED_CMD_BUF_SIZE];
    char line_buf[STR_BUF_SIZE];
    uint8_t y;

    /* ---------- 一次性软件初始化 ---------- */
    if (first_run) {
        OLED_Init();
        HAL_Delay(50);
        OLED_Clear();
        HAL_Delay(20);
        OLED_Clear();

        // 启动画面
        OLED_ShowStr(0, 0, "  System Boot", FONT_SIZE);
        OLED_ShowStr(0, 2, "  Success!", FONT_SIZE);
        osDelay(2000);
        OLED_Clear();
        first_run = 0;
        need_refresh = 1;   // 启动后立即刷新（显示空白或首条数据）
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
                    if (item_count < MAX_TOTAL_ITEMS) {   // 插入新项
                        idx = item_count++;
                        strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
                        items[idx].value = cmd.value;
                        items[idx].active = 1;
                        // 首字母大写
                        if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z')
                            items[idx].label[0] -= 32;
                    } else {                                // 表满，移除最早项
                        for (uint8_t i = 0; i < MAX_TOTAL_ITEMS - 1; i++)
                            items[i] = items[i + 1];
                        idx = MAX_TOTAL_ITEMS - 1;
                        strncpy(items[idx].label, cmd.label, sizeof(items[idx].label) - 1);
                        items[idx].value = cmd.value;
                        if (items[idx].label[0] >= 'a' && items[idx].label[0] <= 'z')
                            items[idx].label[0] -= 32;
                    }
                }
                need_refresh = 1;   // 数据变化，置刷新标志
            }
        }

        /* 2. 自动翻页逻辑 */
        if (HAL_GetTick() - last_scroll_tick > 3000) {
            last_scroll_tick = HAL_GetTick();
            if (item_count > MAX_VISIBLE_ROWS) {
                scroll_offset += MAX_VISIBLE_ROWS;
                if (scroll_offset >= item_count) {
                    scroll_offset = 0;
                }
                need_refresh = 1;   // 翻页触发刷新
            }
        }

        /* 3. 仅在需要时刷新屏幕 */
        if (need_refresh) {
            // 先清空当前显示的4行（避免短字符残留）
            for (y = 0; y < MAX_VISIBLE_ROWS; y++) {
                OLED_ShowStr(0, y * 2, "               ", FONT_SIZE);
            }

            // 绘制实际数据
            for (uint8_t i = 0; i < MAX_VISIBLE_ROWS; i++) {
                uint8_t idx = scroll_offset + i;
                if (idx >= item_count) break;

                snprintf(line_buf, STR_BUF_SIZE, "%.10s:%.3f", items[idx].label, (double)items[idx].value);
                line_buf[MAX_CHARS_LINE] = '\0';
                OLED_ShowStr(0, i * 2, line_buf, FONT_SIZE);
            }
            need_refresh = 0;   // 清除刷新标志
        }

        osDelay(REFRESH_MS);   // 控制循环速度，不影响刷新频率
    }
}
