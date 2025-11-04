#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "ui.h"  // 引入 UI 头文件
#include "encoder_input.h"
#include "eez-flow.h"
#include "led_control.h"
#include "vars.h"
#include <stdint.h>
#include "actions.h"
#include "rc522_reader.h"


/* LCD size */
#define EXAMPLE_LCD_H_RES   (172)
#define EXAMPLE_LCD_V_RES   (320)

/* LCD settings */
#define EXAMPLE_LCD_SPI_NUM         (SPI3_HOST)
#define EXAMPLE_LCD_PIXEL_CLK_HZ    (20 * 1000 * 1000)
#define EXAMPLE_LCD_CMD_BITS        (8)
#define EXAMPLE_LCD_PARAM_BITS      (8)
#define EXAMPLE_LCD_BITS_PER_PIXEL  (16)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_BL_ON_LEVEL     (1)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_SCLK       (GPIO_NUM_11)
#define EXAMPLE_LCD_GPIO_MOSI       (GPIO_NUM_12)
#define EXAMPLE_LCD_GPIO_RST        (GPIO_NUM_14)
#define EXAMPLE_LCD_GPIO_DC         (GPIO_NUM_13)
#define EXAMPLE_LCD_GPIO_CS         (GPIO_NUM_10)
#define EXAMPLE_LCD_GPIO_BL         (GPIO_NUM_45)

static const char *TAG = "MAIN";

static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static lv_display_t *lvgl_disp = NULL;

/* 初始化 LCD */
static esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SCLK,
        .mosi_io_num = EXAMPLE_LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_GPIO_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
#endif
        .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    // esp_lcd_panel_mirror(lcd_panel, true, false);
    esp_lcd_panel_set_gap(lcd_panel, 0, 34);  // 修正 172x320 偏移
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    gpio_set_level(EXAMPLE_LCD_GPIO_BL, EXAMPLE_LCD_BL_ON_LEVEL);
    return ret;

err:
    if (lcd_panel) esp_lcd_panel_del(lcd_panel);
    if (lcd_io) esp_lcd_panel_io_del(lcd_io);
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}

/* 初始化 LVGL */
static esp_err_t app_lvgl_init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 8192,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = { .buff_dma = true, .swap_bytes = true }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    lv_disp_set_rotation(lvgl_disp, LV_DISPLAY_ROTATION_270);
    return ESP_OK;
}










static void rc522_display_task(void *pvParameter)
{
    while (1) {
        const char *text = rc522_get_card_status() ? "读到卡了!" : "没读到卡";
        ESP_LOGI(TAG, "%s",text);
        set_var_read_val(text);

        vTaskDelay(pdMS_TO_TICKS(200)); // 每 200ms 刷新一次
        
    }
}




/* 主函数入口 */
void app_main(void)
{
 
    led_init() ; 
    
    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());

        // 注册编码器
    encoder_init(lvgl_disp);
    rc522_reader_init();
      xTaskCreate(rc522_display_task, "rc522_display", 2048, NULL, 5, NULL);



    ui_init() ; 

    

    while (1)
    {
                ui_tick() ; 
                  vTaskDelay(pdMS_TO_TICKS(10));
    }
    





}
