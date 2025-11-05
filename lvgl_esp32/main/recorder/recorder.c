// inmp441.c
#include "recorder.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

static const char *TAG = "inmp441";

// === 可根据硬件修改这些引脚 ===
#define I2S_BCLK_PIN   GPIO_NUM_7
#define I2S_WS_PIN     GPIO_NUM_48
#define I2S_DIN_PIN    GPIO_NUM_16   // 注意：这是麦克风的 DOUT 输入到 ESP32-S3

#define I2S_PORT       I2S_NUM_0

static i2s_chan_handle_t i2s_rx_handle = NULL;
static bool is_inited = false;

bool inmp441_init(uint32_t sample_rate)
{
    if (is_inited) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // 创建 I2S 通道（仅 RX）
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 128; // 每个 DMA 缓冲区 128 帧
    esp_err_t ret = i2s_new_channel(&chan_cfg, &i2s_rx_handle, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        return false;
    }

    // 配置标准 I2S 模式（用于麦克风）
    i2s_std_config_t std_cfg = {
        .clk_cfg = i2s_std_clk_config_default(sample_rate),
        .slot_cfg = I2S_STD_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_MASK_ALL),
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

    // 关键配置：INMP441 输出 24-bit 数据左对齐在 32-bit 中
    std_cfg.slot_cfg.data_bit_width = I2S_DATA_BIT_WIDTH_32BIT;
    std_cfg.slot_cfg.slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT;
    std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
    std_cfg.slot_cfg.ws_width = I2S_WS_WIDTH_SLOT;
    std_cfg.slot_cfg.ws_pol = I2S_WS_POL_HIGH;   // INMP441: WS=1 表示左声道（单声道无影响）
    std_cfg.slot_cfg.bit_shift = false;          // 不移位（保持左对齐）
    std_cfg.slot_cfg.msb_right = false;          // MSB first（标准 I2S）

    ret = i2s_channel_init_std_mode(i2s_rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S std mode: %s", esp_err_to_name(ret));
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
        return false;
    }

    ret = i2s_channel_enable(i2s_rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S channel: %s", esp_err_to_name(ret));
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