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
#include <ctype.h>
#include "i2c.h"
#include "oled.h"
/* 数据来源头文件 */
#include "angle_sensor.h"
#include "motor.h"
#include "balance.h"
/* ========== 全局命令缓冲区（保留，但不再被任务使用） ========== */
#define OLED_CMD_BUF_SIZE   32
volatile char   oled_cmd_str[OLED_CMD_BUF_SIZE];
volatile uint8_t oled_cmd_new = 0;
/* 旧的推送接口（保留，以避免其他文件编译错误） */
void oled_show(const char *label, float value)
{
    snprintf((char *)oled_cmd_str, OLED_CMD_BUF_SIZE, "%s:%.2f", label, value);
    oled_cmd_new = 1;
}
/* ========== 内部类型 ========== */
typedef struct {
    char    label[16];
    float   value;
} CmdData_t;
typedef struct {
    char    label[16];
    float   value;
    uint8_t active;
} DisplayItem_t;
/* ========== 安全的字符串比较（忽略大小写） ========== */
static int str_cmp_nocase(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 1;
        a++; b++;
    }
    return (*a == *b) ? 0 : 1;
}
/* ========== 更新或插入一个显示项 ========== */
static void update_item(DisplayItem_t items[], uint8_t *count,
                        const char *label, float value, uint8_t max_items)
{
    /* 查找是否已存在 */
    for (uint8_t i = 0; i < *count; i++) {
        if (str_cmp_nocase(items[i].label, label) == 0) {
            items[i].value = value;
            return;
        }
    }
    /* 新增 */
    if (*count < max_items) {
        uint8_t idx = (*count)++;
        strncpy(items[idx].label, label, sizeof(items[idx].label) - 1);
        items[idx].label[sizeof(items[idx].label) - 1] = '\0';
        items[idx].value = value;
        items[idx].active = 1;
        /* 首字母转大写，其余小写 */
        for (int j = 0; items[idx].label[j]; j++) {
            if (j == 0)
                items[idx].label[j] = toupper((unsigned char)items[idx].label[j]);
            else
                items[idx].label[j] = tolower((unsigned char)items[idx].label[j]);
        }
    }
}
/* ========== OLED 任务 ========== */
void oled_f(void const * argument)
{
    #define MAX_VISIBLE_ROWS   4
    #define MAX_TOTAL_ITEMS    12
    #define STR_BUF_SIZE       32
    #define REFRESH_MS         30
    #define FONT_SIZE          16
    #define MAX_CHARS_LINE     15
    static DisplayItem_t items[MAX_TOTAL_ITEMS] = {0};
    static uint8_t       item_count = 0;
    static uint8_t       first_run  = 1;
    static uint8_t       scroll_offset = 0;
    static uint32_t      last_scroll_tick = 0;
    static uint32_t      last_update_tick = 0;
    static uint8_t       need_refresh = 1;
    char line_buf[STR_BUF_SIZE];
    uint8_t y;
    /* 启动屏幕 */
    if (first_run) {
        OLED_Init();
        HAL_Delay(50);
        OLED_Clear();
        HAL_Delay(20);
        OLED_Clear();
        OLED_ShowStr(0, 0, "  System Boot", FONT_SIZE);
        OLED_ShowStr(0, 2, "  Success!", FONT_SIZE);
        osDelay(2000);
        OLED_Clear();
        first_run = 0;
        need_refresh = 1;
    }
    for (;;) {
        /* 1. 主动数据获取（500ms） */
        if (HAL_GetTick() - last_update_tick > 500) {
            last_update_tick = HAL_GetTick();
            // 从传感器队列 Peek（不删除）
            Sensor_Data_Typedef sensor;
            if (xQueuePeek(xSensorQueue, &sensor, 0) == pdTRUE) {
                update_item(items, &item_count, "Angle1", (float)sensor.angle1, MAX_TOTAL_ITEMS);
                update_item(items, &item_count, "Angle2", (float)sensor.angle2, MAX_TOTAL_ITEMS);
                update_item(items, &item_count, "AVel1",  (float)sensor.angular_velocity1, MAX_TOTAL_ITEMS);
                update_item(items, &item_count, "AVel2",  (float)sensor.angular_velocity2, MAX_TOTAL_ITEMS);
            }
            // 从电机反馈队列 Peek
            MotorFeedback_t motor_fb;
            if (xQueuePeek(xMotorFeedbackQueue, &motor_fb, 0) == pdTRUE) {
                update_item(items, &item_count, "Speed",  (float)motor_fb.speed_rpm, MAX_TOTAL_ITEMS);
                update_item(items, &item_count, "EncPos", (float)motor_fb.encoder_pos, MAX_TOTAL_ITEMS);
                update_item(items, &item_count, "AngleM", (float)motor_fb.angle_deg, MAX_TOTAL_ITEMS);
            }
            // 运行状态
            extern volatile uint8_t RunState;
            update_item(items, &item_count, "RunState", (float)RunState, MAX_TOTAL_ITEMS);
            need_refresh = 1;   // 标记重绘
        }
        /* 2. 自动翻页（3秒） */
        if (HAL_GetTick() - last_scroll_tick > 3000) {
            last_scroll_tick = HAL_GetTick();
            if (item_count > MAX_VISIBLE_ROWS) {
                scroll_offset += MAX_VISIBLE_ROWS;
                if (scroll_offset >= item_count) {
                    scroll_offset = 0;
                }
                need_refresh = 1;
            }
        }
        /* 3. 刷新屏幕 */
        if (need_refresh) {
            // 清空可见行
            for (y = 0; y < MAX_VISIBLE_ROWS; y++) {
                OLED_ShowStr(0, y * 2, "               ", FONT_SIZE);
            }
            // 显示当前页
            for (uint8_t i = 0; i < MAX_VISIBLE_ROWS; i++) {
                uint8_t idx = scroll_offset + i;
                if (idx >= item_count) break;
                snprintf(line_buf, STR_BUF_SIZE, "%.8s:%.1f", items[idx].label, (double)items[idx].value);
                line_buf[MAX_CHARS_LINE] = '\0';
                OLED_ShowStr(0, i * 2, line_buf, FONT_SIZE);
            }
            need_refresh = 0;
        }
        osDelay(REFRESH_MS);
    }
}
