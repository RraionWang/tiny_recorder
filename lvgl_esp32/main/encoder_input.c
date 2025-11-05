#include "encoder_input.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_timer.h"  // 用于高精度时间戳

#include "pin_cng.h"



#define DEBOUNCE_TIME_MS    10  // 防抖时间（毫秒），可根据实际调整

// 原始状态（由 ISR 快速记录）
static volatile int8_t encoder_raw_diff = 0;
static volatile bool encoder_raw_pressed = false;

// 去抖后的状态（由 LVGL 读取回调使用）
static int8_t encoder_debounced_diff = 0;
static bool encoder_debounced_pressed = false;

// 上次有效变化的时间戳（微秒）
static int64_t last_encoder_change_us = 0;
static int64_t last_button_change_us = 0;

/* A/B 相位中断 —— 只记录原始电平变化，不做防抖 */
static void IRAM_ATTR encoder_isr_handler(void *arg)
{
    bool a = gpio_get_level(ENCODER_A_PIN);
    bool b = gpio_get_level(ENCODER_B_PIN);
    if (a == b)
        encoder_raw_diff++;
    else
        encoder_raw_diff--;
    last_encoder_change_us = esp_timer_get_time();  // 记录最新变化时间
}

/* 按键中断 —— 只记录原始电平 */
static void IRAM_ATTR encoder_btn_isr(void *arg)
{
    encoder_raw_pressed = !gpio_get_level(ENCODER_BTN_PIN);
    last_button_change_us = esp_timer_get_time();
}

/* LVGL 输入读取回调 —— 在这里做防抖 */
static void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    int64_t now = esp_timer_get_time();  // 单位：微秒

    // ===== 按键防抖 =====
    if (now - last_button_change_us >= DEBOUNCE_TIME_MS * 1000) {
        encoder_debounced_pressed = encoder_raw_pressed;
    }
    // 否则保持上次去抖后的状态（不更新）

    // ===== 编码器旋转防抖 =====
    // 注意：旋转事件是累积的，我们只在稳定后才“提交”增量
    if (now - last_encoder_change_us >= DEBOUNCE_TIME_MS * 1000) {
        encoder_debounced_diff = encoder_raw_diff;
        encoder_raw_diff = 0;  // 清空原始计数（已提交）
    }
    // 如果还在抖动期，不提交新值，也不清空 raw_diff

    // 提供给 LVGL
    data->enc_diff = encoder_debounced_diff;
    data->state = encoder_debounced_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    // 清空已提交的去抖值（避免重复发送）
    encoder_debounced_diff = 0;
}

/* 初始化编码器输入设备 */
void encoder_init(lv_display_t *disp)
{
    /* 配置旋转编码器 A/B */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << ENCODER_A_PIN) | (1ULL << ENCODER_B_PIN),
        .pull_up_en = true,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_A_PIN, encoder_isr_handler, NULL);
    gpio_isr_handler_add(ENCODER_B_PIN, encoder_isr_handler, NULL);

    /* 配置按钮 */
    gpio_config_t btn_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << ENCODER_BTN_PIN,
        .pull_up_en = true,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&btn_conf);
    gpio_isr_handler_add(ENCODER_BTN_PIN, encoder_btn_isr, NULL);

    /* LVGL v9 输入设备注册 */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, encoder_read_cb);

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(indev, group);
}