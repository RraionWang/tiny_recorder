// rc522_reader.c
#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "rc522_reader.h"

static const char *TAG = "RC522_READER";

// 引脚定义（ESP32-S3）
#define RC522_SPI_BUS_GPIO_MISO    (17)
#define RC522_SPI_BUS_GPIO_MOSI    (18)
#define RC522_SPI_BUS_GPIO_SCLK    (41)
#define RC522_SCANNER_GPIO_SDA     (42)
#define RC522_SCANNER_GPIO_RST     (15)

// 全局状态
static bool g_card_present = false;
static SemaphoreHandle_t g_card_mutex = NULL;

// 驱动和扫描器句柄
static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

// 卡片状态变化回调
static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    xSemaphoreTake(g_card_mutex, portMAX_DELAY);
    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "✅ 检测到卡片");
        rc522_picc_print(picc);
        g_card_present = true;
    } else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "💨 卡片已移开");
        g_card_present = false;
    }
    xSemaphoreGive(g_card_mutex);
}

void rc522_reader_init(void)
{
    ESP_LOGI(TAG, "🔧 初始化 RC522 (SPI 模式)");

    // 创建互斥锁
    g_card_mutex = xSemaphoreCreateMutex();
    configASSERT(g_card_mutex);

    // SPI 驱动配置
    rc522_spi_config_t driver_config = {
        .host_id = SPI2_HOST,
        .bus_config = &(spi_bus_config_t){
            .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
            .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
            .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
        },
        .dev_config = {
            .spics_io_num = RC522_SCANNER_GPIO_SDA,
            .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        },
        .rst_io_num = RC522_SCANNER_GPIO_RST,
    };

    // 创建并安装驱动
    ESP_ERROR_CHECK(rc522_spi_create(&driver_config, &driver));
    ESP_ERROR_CHECK(rc522_driver_install(driver));

    // 创建扫描器
    rc522_config_t scanner_config = {
        .driver = driver,
    };
    ESP_ERROR_CHECK(rc522_create(&scanner_config, &scanner));

    // 注册事件回调
    ESP_ERROR_CHECK(rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL));

    // 启动轮询
    ESP_ERROR_CHECK(rc522_start(scanner));

    ESP_LOGI(TAG, "📡 RC522 初始化完成，请将卡靠近天线...");
}

bool rc522_get_card_status(void)
{
    bool status = false;
    if (g_card_mutex) {
        xSemaphoreTake(g_card_mutex, portMAX_DELAY);
        status = g_card_present;
        xSemaphoreGive(g_card_mutex);
    }
    return status;
}