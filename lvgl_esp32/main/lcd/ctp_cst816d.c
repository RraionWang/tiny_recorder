#include "ctp_cst816d.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "pin_cng.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ========== 配置参数 ========== */
#define CTP_I2C_FREQ_HZ        400000
#define CTP_I2C_ADDR           0x15

#define TP_H_RES               172
#define TP_V_RES               320

#define TP_SWAP_XY             0
#define TP_MIRROR_X            0
#define TP_MIRROR_Y            0
#define TP_GAP_X               0
#define TP_GAP_Y               34

static const char *TAG = "CTP(CST816D)";
static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;
static lv_indev_t *s_indev = NULL;

/* ========== 基础I2C操作函数 ========== */
static esp_err_t ctp_reg_read(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, data, len, 1000);
}

static esp_err_t ctp_reg_write(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 1000);
}

/* ========== 触摸复位 ========== */
static void ctp_reset(void)
{
    gpio_config_t rst = {
        .pin_bit_mask = (1ULL << CTP_PIN_RST),
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&rst);
    gpio_set_level(CTP_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(CTP_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

/* ========== 坐标映射 ========== */
static void tp_map_to_lvgl(uint16_t rx, uint16_t ry, lv_point_t *pt)
{
    int16_t x = rx, y = ry;
    x -= TP_GAP_X; if (x < 0) x = 0;
    y -= TP_GAP_Y; if (y < 0) y = 0;

#if TP_SWAP_XY
    int16_t t = x; x = y; y = t;
#endif
#if TP_MIRROR_X
    x = (TP_H_RES - 1) - x;
#endif
#if TP_MIRROR_Y
    y = (TP_V_RES - 1) - y;
#endif
    if (x >= TP_H_RES) x = TP_H_RES - 1;
    if (y >= TP_V_RES) y = TP_V_RES - 1;

    pt->x = x;
    pt->y = y;
}

/* ========== 读取触摸坐标 ========== */
static bool ctp_read_once(lv_point_t *pt)
{
    uint8_t buf[7] = {0};
    if (ctp_reg_read(0x01, buf, sizeof(buf)) != ESP_OK) return false;

    uint8_t points = buf[1] & 0x0F;
    if (points == 0) return false;

    uint16_t x = ((buf[2] & 0x0F) << 8) | buf[3];
    uint16_t y = ((buf[4] & 0x0F) << 8) | buf[5];

    tp_map_to_lvgl(x, y, pt);

    ESP_LOGI("LVGL","x=%d,y=%d",x,y);
    return true;
}

/* ========== LVGL回调 ========== */
static void ctp_lvgl_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    lv_point_t p;
    if (ctp_read_once(&p)) {
        data->point = p;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ========== 公共接口实现 ========== */
esp_err_t ctp_init(void)
{
    // 1. 初始化I2C总线
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .scl_io_num = CTP_PIN_SCL,
        .sda_io_num = CTP_PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c bus create failed");

    // 2. 绑定CST816D设备
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CTP_I2C_ADDR,
        .scl_speed_hz = CTP_I2C_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev), TAG, "add device failed");

    // 3. 复位触摸芯片
    ctp_reset();

    // 4. 读取芯片ID（可选）
    uint8_t id = 0;
    if (ctp_reg_read(0xA7, &id, 1) == ESP_OK)
        ESP_LOGI(TAG, "CST816D ID: 0x%02X", id);

    ESP_LOGI(TAG, "✅ CST816D 初始化完成");
    return ESP_OK;
}

lv_indev_t *ctp_register_lvgl(lv_display_t *disp)
{
    s_indev = lv_indev_create();
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, ctp_lvgl_read_cb);
    lv_indev_set_display(s_indev, disp);
    ESP_LOGI(TAG, "LVGL 输入设备注册完成");
    return s_indev;
}
