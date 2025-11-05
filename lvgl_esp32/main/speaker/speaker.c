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

/* ========= 引脚定义 ========= */
#define I2S_BCLK    7
#define I2S_LRCLK   8
#define I2S_DOUT    9
#define AMP_SD_PIN  10          // 可选：若模块有使能引脚则使用，否则可忽略

#define SD_MOSI     13
#define SD_MISO     14
#define SD_SCK      12
#define SD_CS       11

#define DEFAULT_SAMPLE_RATE 44100
#define BUFFER_SIZE         4096

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

/* === SD 卡初始化 === */
static esp_err_t sdcard_init(void)
{
    esp_err_t ret;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_CS;
    slot_cfg.host_id = host.slot;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
    };

    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &card);
    if (ret == ESP_OK) {
        sdmmc_card_print_info(stdout, card);
    }
    return ret;
}

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
void wav_player_play(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        printf("❌ 打开文件失败: %s\n", path);
        return;
    }

    wav_header_t header;
    if (fread(&header, sizeof(wav_header_t), 1, fp) != 1) {
        printf("❌ 读取 WAV 头失败\n");
        fclose(fp);
        return;
    }

    printf("🎵 WAV: %lu Hz, %u bit, %u ch\n",
           (unsigned long)header.sample_rate,
           header.bits_per_sample,
           header.num_channels);

    if (header.audio_format != 1 || header.bits_per_sample != 16) {
        printf("⚠️ 仅支持 16-bit PCM WAV\n");
        fclose(fp);
        return;
    }

    if (header.sample_rate != DEFAULT_SAMPLE_RATE) {
        reconfigure_sample_rate(header.sample_rate);
        printf("🔧 重新配置 I2S 采样率为 %lu Hz\n", (unsigned long)header.sample_rate);
    }

    uint8_t *buf = malloc(BUFFER_SIZE);
    int16_t *mono_buf = malloc(BUFFER_SIZE / 2);
    if (!buf || !mono_buf) {
        printf("❌ 内存分配失败\n");
        free(buf);
        free(mono_buf);
        fclose(fp);
        return;
    }

    const float volume = 0.6f;
    size_t bytes_read, bytes_written;

    gpio_set_level(AMP_SD_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    while ((bytes_read = fread(buf, 1, BUFFER_SIZE, fp)) > 0) {
        size_t samples_out = 0;
        if (header.num_channels == 2) {
            int16_t *p = (int16_t *)buf;
            size_t frames = bytes_read / 4;
            for (size_t i = 0; i < frames; i++) {
                float mixed = (p[2 * i] + p[2 * i + 1]) * 0.5f * volume;
                if (mixed > 32767) mixed = 32767;
                if (mixed < -32768) mixed = -32768;
                mono_buf[samples_out++] = (int16_t)mixed;
            }
        } else {
            int16_t *p = (int16_t *)buf;
            size_t samples = bytes_read / 2;
            for (size_t i = 0; i < samples; i++) {
                float s = p[i] * volume;
                if (s > 32767) s = 32767;
                if (s < -32768) s = -32768;
                mono_buf[samples_out++] = (int16_t)s;
            }
        }

        i2s_channel_write(tx_chan, mono_buf, samples_out * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    }

    gpio_set_level(AMP_SD_PIN, 0);
    free(buf);
    free(mono_buf);
    fclose(fp);
    printf("✅ 播放结束\n");
}

/* === 初始化函数 === */
bool wav_player_init(void)
{
    // 配置功放使能引脚（如有）
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << AMP_SD_PIN,
    };
    gpio_config(&io_conf);
    gpio_set_level(AMP_SD_PIN, 0);

    printf("🧩 初始化 SD 卡...\n");
    ESP_ERROR_CHECK_WITHOUT_ABORT(sdcard_init());

    printf("🎧 初始化 I2S...\n");
    if (i2s_init(DEFAULT_SAMPLE_RATE) != ESP_OK) {
        printf("❌ I2S 初始化失败\n");
        return false;
    }

    return true;
}