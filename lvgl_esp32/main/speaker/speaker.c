#include "speaker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/spi_common.h"
#include "pin_cfg.h"
#include "sdcard.h"

/* ========= 引脚定义 ========= */
#define I2S_BCLK    13
#define I2S_LRCLK   14
#define I2S_DOUT    12
// #define AMP_SD_PIN  -1          // 可选：若模块有使能引脚则使用，否则可忽略

#define DEFAULT_SAMPLE_RATE 44100
#define BUFFER_SIZE         4096

static const char* TAG = "NS4168" ; 

static i2s_chan_handle_t tx_chan = NULL;

/* ====== WAV 文件头结构体 ====== */
typedef struct {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} wav_header_t;


/* === I2S 初始化 === */
static esp_err_t i2s_init(uint32_t sample_rate)
{
    i2s_chan_config_t tx_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&tx_cfg, &tx_chan, NULL), TAG, "创建 I2S 通道失败");

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK,
            .ws   = I2S_LRCLK,
            .dout = I2S_DOUT,
            .din  = I2S_GPIO_UNUSED,
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_chan, &std_cfg), TAG, "I2S 标准模式初始化失败");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_chan), TAG, "启用 I2S 通道失败");
    return ESP_OK;
}

/* === 重新配置采样率（仅时钟）=== */
static void reconfigure_sample_rate(uint32_t new_rate)
{
    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(new_rate);
    i2s_channel_disable(tx_chan);
    i2s_channel_reconfig_std_clock(tx_chan, &clk_cfg);
    i2s_channel_enable(tx_chan);
}

/* === 播放 WAV 文件 === */
/* === 播放 WAV 文件（修复版：使用静态缓冲区，避免堆损坏）=== */
void wav_player_play(const char *path)
{
    // 使用静态缓冲区，确保生命周期覆盖整个播放过程，且位于内部 RAM（DMA-safe）
    static uint8_t buf[BUFFER_SIZE];
    static int16_t mono_buf[BUFFER_SIZE / 2];  // 最多处理 BUFFER_SIZE/2 个 16-bit 样点

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "❌ 打开文件失败: %s", path);
        return;
    }

    wav_header_t header;
    if (fread(&header, sizeof(wav_header_t), 1, fp) != 1) {
        ESP_LOGE(TAG, "❌ 读取 WAV 头失败");
        fclose(fp);
        return;
    }

    ESP_LOGI(TAG, "🎵 WAV: %lu Hz, %u bit, %u ch",
             (unsigned long)header.sample_rate,
             header.bits_per_sample,
             header.num_channels);

    if (header.audio_format != 1 || header.bits_per_sample != 16) {
        ESP_LOGW(TAG, "⚠️ 仅支持 16-bit PCM WAV");
        fclose(fp);
        return;
    }

    if (header.sample_rate != DEFAULT_SAMPLE_RATE) {
        reconfigure_sample_rate(header.sample_rate);
        ESP_LOGI(TAG, "🔧 重新配置 I2S 采样率为 %lu Hz", (unsigned long)header.sample_rate);
    }

    const float volume = 0.6f;
    size_t bytes_read, bytes_written;

    vTaskDelay(pdMS_TO_TICKS(100)); // 给功放/硬件一点启动时间（如有）

    while ((bytes_read = fread(buf, 1, BUFFER_SIZE, fp)) > 0) {
        size_t samples_out = 0;

        if (header.num_channels == 2) {
            int16_t *p = (int16_t *)buf;
            size_t frames = bytes_read / 4; // 2 channels × 2 bytes
            for (size_t i = 0; i < frames && i < BUFFER_SIZE / 4; i++) {
                float mixed = (p[2 * i] + p[2 * i + 1]) * 0.5f * volume;
                if (mixed > 32767.0f) mixed = 32767.0f;
                if (mixed < -32768.0f) mixed = -32768.0f;
                mono_buf[samples_out++] = (int16_t)mixed;
            }
        } else {
            int16_t *p = (int16_t *)buf;
            size_t samples = bytes_read / 2;
            for (size_t i = 0; i < samples && i < BUFFER_SIZE / 2; i++) {
                float s = p[i] * volume;
                if (s > 32767.0f) s = 32767.0f;
                if (s < -32768.0f) s = -32768.0f;
                mono_buf[samples_out++] = (int16_t)s;
            }
        }

        // 阻塞写入，等待 DMA 描述符入队（注意：不等于播放完成，但静态 buffer 安全）
        esp_err_t ret = i2s_channel_write(tx_chan, mono_buf, samples_out * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "I2S 写入失败: %s", esp_err_to_name(ret));
            break;
        }
    }

    // 可选：等待传输完成（更严谨）
    // 注意：ESP-IDF v5.5 的 I2S channel API 暂无直接 flush，但关闭再开启可清空 FIFO
    i2s_channel_disable(tx_chan);
    i2s_channel_enable(tx_chan);

    fclose(fp);
    ESP_LOGI(TAG, "✅ 播放结束: %s", path);
}

/* === 初始化函数 === */
bool wav_player_init(void)
{
   

    printf("🎧 初始化 I2S...\n");
    if (i2s_init(DEFAULT_SAMPLE_RATE) != ESP_OK) {
        printf("❌ I2S 初始化失败\n");
        return false;
    }

    return true;
}