
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// 必须包含 ESP-IDF 的基础头文件，它会自动引入 FreeRTOS 类型定义
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"        // 如果用到队列
#include "driver/i2s_std.h"        // 根据你用的 I2S 驱动版本调整
#include "esp_log.h"

static const char *TAG = "inmp441";

#define I2S_BCLK_PIN   GPIO_NUM_4
#define I2S_WS_PIN     GPIO_NUM_5
#define I2S_DIN_PIN    GPIO_NUM_6
#define I2S_PORT       I2S_NUM_0

static i2s_chan_handle_t i2s_rx_handle = NULL;
static bool is_inited = false;

bool inmp441_init(uint32_t sample_rate)
{
    if (is_inited) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // 1. 创建 I2S RX 通道
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 128;
    esp_err_t ret = i2s_new_channel(&chan_cfg, &i2s_rx_handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(ret));
        return false;
    }

    // 2. 使用宏初始化 slot_cfg（注意：只有 2 个参数！）
    i2s_std_slot_config_t slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_32BIT,
        I2S_SLOT_MODE_MONO
    );

    // 手动覆盖关键字段以适配 INMP441
    slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;  // 或 I2S_STD_SLOT_RIGHT，取决于 L/R 引脚接法
    slot_cfg.bit_shift = false;              // 关键：保持 24-bit 左对齐在 32-bit 中
    // 注意：v5.5 中没有 msb_first 字段，默认就是 MSB first（Philips 模式）

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DIN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(i2s_rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(ret));
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
        return false;
    }

    ret = i2s_channel_enable(i2s_rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(ret));
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
        return false;
    }

    is_inited = true;
    ESP_LOGI(TAG, "INMP441 initialized at %lu Hz", sample_rate);
    return true;
}

size_t inmp441_read(int32_t *buffer, size_t num_samples, uint32_t timeout_ms)
{
    if (!is_inited || !buffer || num_samples == 0) {
        return 0;
    }

    size_t bytes_to_read = num_samples * sizeof(int32_t);
    size_t bytes_read = 0;

    esp_err_t ret = i2s_channel_read(i2s_rx_handle, buffer, bytes_to_read, &bytes_read, pdMS_TO_TICKS(timeout_ms));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return bytes_read / sizeof(int32_t);
}

void inmp441_deinit(void)
{
    if (!is_inited) return;

    if (i2s_rx_handle) {
        i2s_channel_disable(i2s_rx_handle);
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
    }
    is_inited = false;
    ESP_LOGI(TAG, "INMP441 deinitialized");
}