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
#include "speaker.h"
#include "recorder.h"
#include "vars.h"
#include "ui.h"
#include "screens.h"

static const char *TAG = "RC522_READER";


static SemaphoreHandle_t g_card_mutex = NULL;

// 驱动和扫描器句柄
static rc522_driver_handle_t driver;
static rc522_handle_t scanner;

#define MAX_UID_HEX_LEN 24  // 最多支持 12 字节 UID（实际一般 ≤10）
static char g_current_uid[RC522_PICC_UID_STR_BUFFER_SIZE_MAX] = {0};
static bool g_is_recording_for_card = false;

// 将 rc522_uid_t 转为连续 hex 字符串（无空格）
static void uid_to_hex_str(const rc522_picc_uid_t *uid, char *out_str, size_t out_size)
{
    if (!uid || !out_str || out_size < (uid->length * 2 + 1)) {
        if (out_str && out_size > 0) out_str[0] = '\0';
        return;
    }

    for (uint8_t i = 0; i < uid->length; i++) {
        snprintf(&out_str[i * 2], out_size - i * 2, "%02X", uid->value[i]);
    }
}


static bool file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

// 播放任务：接收 filepath 的副本
static void play_wav_task(void *pvParam)
{
    char *filepath = (char *)pvParam;
    ESP_LOGI("Audio", "▶️ 开始播放: %s", filepath);
    
    wav_player_play(filepath); // 假设这个函数会阻塞直到播放结束
    
    free(filepath); // 因为是从 strdup 分配的
    vTaskDelete(NULL);
}



static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    if (xSemaphoreTake(g_card_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE("RFID", "Mutex timeout");
        return;
    }

    if (picc->state == RC522_PICC_STATE_ACTIVE) {



        ESP_LOGI("RFID", "检测到卡片了");
        set_var_is_detected_rfid_new_card(true);
        ESP_LOGI("RFID", "变量设置为 %d", get_var_is_detected_rfid_new_card());

        // 这个函数可以用来切换屏幕
        eez_flow_set_screen(SCREEN_ID_DETECTED_RFID_PAGE,LV_SCREEN_LOAD_ANIM_NONE,200,0);

        

        // tick_screen_detected_rfid_page(); 



        // // ✅ 使用无空格的 HEX 字符串
        // char uid_hex[32] = {0};
        // for (uint8_t i = 0; i < picc->uid.length && i < 10; i++) {
        //     snprintf(&uid_hex[i * 2], sizeof(uid_hex) - i * 2, "%02X", picc->uid.value[i]);
        // }

        // char filepath[128];
        // snprintf(filepath, sizeof(filepath), "/sdcard/%s.wav", uid_hex);

        // ESP_LOGI("RFID", "检测到卡片，UID: %s", uid_hex);



    } else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        // if (g_is_recording_for_card) {
        //     // stop_recording();
        //     g_is_recording_for_card = false;
        //     memset(g_current_uid, 0, sizeof(g_current_uid));
        // }
                // set_var_is_detected_rfid_new_card(false);
    }

    xSemaphoreGive(g_card_mutex);
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

