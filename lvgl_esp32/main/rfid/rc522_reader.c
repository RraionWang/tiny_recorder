// rc522_reader.c
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "rc522_reader.h"
#include "pin_cng.h"

static const char *TAG = "RC522_READER";


static SemaphoreHandle_t g_card_mutex = NULL;

// 驱动和扫描器句柄
static rc522_driver_handle_t driver;
static rc522_handle_t scanner;



// 直接在回调函数中写
// 卡片状态变化回调
static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    // 回调中通常不会被抢占，可使用 portMAX_DELAY（但建议仍加超时防御）
    if (xSemaphoreTake(g_card_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (picc->state == RC522_PICC_STATE_ACTIVE) {
                char uid_str[RC522_PICC_UID_STR_BUFFER_SIZE_MAX];
    ESP_ERROR_CHECK(rc522_picc_uid_to_str(&picc->uid, uid_str, sizeof(uid_str)));
            ESP_LOGI(TAG, "✅ 检测到卡片%u 卡片的uid是%s", (unsigned int)esp_log_timestamp(),uid_str);
       
        } else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
            ESP_LOGI(TAG, "💨 卡片已移开%u ms", (unsigned int)esp_log_timestamp());
          

        }
        xSemaphoreGive(g_card_mutex);
    } else {
        ESP_LOGE(TAG, "回调中获取 mutex 超时！");
    }
}

void rc522_reader_init(void)
{
    ESP_LOGI(TAG, "🔧 初始化 RC522 (SPI 模式)");

    g_card_mutex = xSemaphoreCreateMutex();
    configASSERT(g_card_mutex);

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
            .clock_speed_hz = 1 * 1000 * 1000,
        },
        .rst_io_num = RC522_SCANNER_GPIO_RST,
    };

    ESP_ERROR_CHECK(rc522_spi_create(&driver_config, &driver));
    ESP_ERROR_CHECK(rc522_driver_install(driver));

    rc522_config_t scanner_config = {
        .driver = driver,
    };
    ESP_ERROR_CHECK(rc522_create(&scanner_config, &scanner));
    ESP_ERROR_CHECK(rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL));
    ESP_ERROR_CHECK(rc522_start(scanner));

    ESP_LOGI(TAG, "📡 RC522 初始化完成，请将卡靠近天线...");
}

