#include "recorder.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "esp_check.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "INMP441";

static void write_wav_header(FILE *f, int sample_rate, int bits_per_sample, int num_channels) {
    uint8_t header[44];
    int byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    int block_align = num_channels * bits_per_sample / 8;
    memset(header, 0, sizeof(header));

    memcpy(header, "RIFF", 4);
    *(uint32_t *)(header + 4) = 0;  // 先填0，最后更新文件大小
    memcpy(header + 8, "WAVEfmt ", 8);
    *(uint32_t *)(header + 16) = 16;
    *(uint16_t *)(header + 20) = 1;  // PCM
    *(uint16_t *)(header + 22) = num_channels;
    *(uint32_t *)(header + 24) = sample_rate;
    *(uint32_t *)(header + 28) = byte_rate;
    *(uint16_t *)(header + 32) = block_align;
    *(uint16_t *)(header + 34) = bits_per_sample;
    memcpy(header + 36, "data", 4);
    *(uint32_t *)(header + 40) = 0;  // 先填0

    fwrite(header, 1, sizeof(header), f);
}

esp_err_t inmp441_init(inmp441_recorder_t *rec) {
    ESP_LOGI(TAG, "Initializing INMP441 I2S...");

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, NULL, &rec->rx_chan), TAG, "create channel failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(SAMPLE_BITS, CHANNEL_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_PIN,
            .ws   = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din  = I2S_DIN_PIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; // INMP441 输出左声道数据
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rec->rx_chan, &std_cfg), TAG, "i2s std init failed");

    ESP_LOGI(TAG, "INMP441 initialized on I2S%d", I2S_PORT);
    return ESP_OK;
}

esp_err_t inmp441_start_record(inmp441_recorder_t *rec, const char *filename) {
    if (rec->is_recording) {
        ESP_LOGW(TAG, "Already recording!");
        return ESP_FAIL;
    }

    snprintf(rec->filepath, sizeof(rec->filepath), "/sdcard/%s", filename);
    rec->file = fopen(rec->filepath, "wb");
    if (!rec->file) {
        ESP_LOGE(TAG, "Failed to open %s for writing", rec->filepath);
        return ESP_FAIL;
    }

    write_wav_header(rec->file, SAMPLE_RATE_HZ, 16, 1);
    ESP_ERROR_CHECK(i2s_channel_enable(rec->rx_chan));
    rec->is_recording = true;

    ESP_LOGI(TAG, "Recording started: %s", rec->filepath);
    xTaskCreate(inmp441_record_task, "inmp441_record_task", 4096, rec, 5, NULL);
    return ESP_OK;
}

void inmp441_record_task(void *param)
{
    inmp441_recorder_t *rec = (inmp441_recorder_t *)param;
    uint8_t *buf = malloc(BUFFER_SIZE);
    int16_t *out_buf = malloc(BUFFER_SIZE / 2);
    size_t bytes_read = 0;
    size_t samples_out = 0;

    ESP_LOGI(TAG, "Recording task running (optimized filter)...");

    // DC offset & 平滑滤波用变量
    float dc_offset = 0.0f;
    const float alpha = 0.995f;     // 高频保留，去除直流偏移
    const float gain = 1.0f;        // 可以调节增益 (0.5 ~ 2.0)

    while (rec->is_recording) {
        esp_err_t ret = i2s_channel_read(rec->rx_chan, buf, BUFFER_SIZE, &bytes_read, 1000);
        if (ret == ESP_OK && bytes_read > 0) {
            int32_t *p = (int32_t *)buf;
            size_t frames = bytes_read / 4;
            samples_out = 0;

      for (size_t i = 0; i < frames; i++) {
    int32_t raw = p[i];             // 直接取原始32bit
    int16_t pcm = (int16_t)(raw >> 11);  // 关键！右移11位是经过验证最合适的缩放
    out_buf[samples_out++] = pcm;
}


            fwrite(out_buf, sizeof(int16_t), samples_out, rec->file);
        } else {
            ESP_LOGW(TAG, "I2S read timeout");
        }
    }

    free(buf);
    free(out_buf);
    ESP_LOGI(TAG, "Recording task stopped");
    vTaskDelete(NULL);
}



void inmp441_stop_record(inmp441_recorder_t *rec) {
    if (!rec->is_recording) return;

    rec->is_recording = false;
    vTaskDelay(pdMS_TO_TICKS(500)); // 等待缓冲区写完
    ESP_ERROR_CHECK(i2s_channel_disable(rec->rx_chan));

    long file_size = ftell(rec->file);
    fseek(rec->file, 4, SEEK_SET);
    uint32_t riff_size = file_size - 8;
    fwrite(&riff_size, 4, 1, rec->file);
    fseek(rec->file, 40, SEEK_SET);
    uint32_t data_size = file_size - 44;
    fwrite(&data_size, 4, 1, rec->file);

    fclose(rec->file);
    rec->file = NULL;

    ESP_LOGI(TAG, "Recording saved to %s, size: %ld bytes", rec->filepath, file_size);
}
