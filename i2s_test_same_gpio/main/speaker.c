#include "speaker.h"
#include "audio.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

static const char *TAG = "SPEAKER";

#define PLAY_BUFFER_SIZE 4096

void wav_player_play(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "❌ 无法打开文件: %s", path);
        return;
    }

    // 跳过 WAV 头
    fseek(fp, 44, SEEK_SET);

    uint8_t *buf = malloc(PLAY_BUFFER_SIZE);
    if (!buf) {
        ESP_LOGE(TAG, "❌ 无法分配播放缓冲区");
        fclose(fp);
        return;
    }

    size_t bytes_read, bytes_written;
    ESP_LOGI(TAG, "🔊 开始播放: %s", path);

    while ((bytes_read = fread(buf, 1, PLAY_BUFFER_SIZE, fp)) > 0) {
        if (i2s_channel_write(tx_chan, buf, bytes_read, &bytes_written, portMAX_DELAY) != ESP_OK) {
            ESP_LOGE(TAG, "I2S 写入失败");
            break;
        }
    }

    free(buf);
    fclose(fp);
    ESP_LOGI(TAG, "✅ 播放结束");
}
