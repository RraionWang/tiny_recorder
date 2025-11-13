#include "recorder.h"
#include "audio.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"

static const char *TAG = "RECORDER";

#define REC_BUFFER_SIZE 4096

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

static void write_wav_header(FILE *fp, int sample_rate, int bits_per_sample, int channels, uint32_t data_size)
{
    wav_header_t header;
    memcpy(header.riff, "RIFF", 4);
    header.chunk_size = data_size + 36;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.subchunk1_size = 16;
    header.audio_format = 1;
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * channels * bits_per_sample / 8;
    header.block_align = channels * bits_per_sample / 8;
    header.bits_per_sample = bits_per_sample;
    memcpy(header.data, "data", 4);
    header.data_size = data_size;

    fseek(fp, 0, SEEK_SET);
    fwrite(&header, 1, sizeof(header), fp);
}

void recorder_record_wav(const char *path, int seconds)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "❌ 打开文件失败: %s", path);
        return;
    }

    // 先写入空的 WAV 头（稍后更新）
    uint8_t empty_header[44] = {0};
    fwrite(empty_header, 1, sizeof(empty_header), fp);

    uint8_t *buf = malloc(REC_BUFFER_SIZE);
    if (!buf) {
        ESP_LOGE(TAG, "❌ 分配录音缓冲失败");
        fclose(fp);
        return;
    }

    size_t bytes_read;
    uint32_t total_bytes = 0;
    ESP_LOGI(TAG, "🎙️ 开始录音 %d 秒...", seconds);

    int loops = (seconds * 44100 * 2) / REC_BUFFER_SIZE;
    for (int i = 0; i < loops; i++) {
        if (i2s_channel_read(rx_chan, buf, REC_BUFFER_SIZE, &bytes_read, portMAX_DELAY) == ESP_OK) {
            fwrite(buf, 1, bytes_read, fp);
            total_bytes += bytes_read;
        }
    }

    ESP_LOGI(TAG, "✅ 录音完成，总字节数: %lu", (unsigned long)total_bytes);

    write_wav_header(fp, 44100, 16, 1, total_bytes);
    fclose(fp);
    free(buf);
}
