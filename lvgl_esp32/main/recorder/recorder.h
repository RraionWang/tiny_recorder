#ifndef RECORDER_H
#define RECORDER_H

#include "driver/i2s_std.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2S_BCLK_PIN   GPIO_NUM_4
#define I2S_WS_PIN     GPIO_NUM_5
#define I2S_DIN_PIN    GPIO_NUM_6
#define I2S_PORT       I2S_NUM_1

#define SAMPLE_RATE_HZ 48000
#define SAMPLE_BITS    I2S_DATA_BIT_WIDTH_32BIT
#define CHANNEL_MODE   I2S_SLOT_MODE_MONO
#define BUFFER_SIZE    2048
#define FILE_PATH_MAX  128

typedef struct {
    i2s_chan_handle_t rx_chan;
    FILE *file;
    bool is_recording;
    char filepath[FILE_PATH_MAX];
} inmp441_recorder_t;

/**
 * @brief 初始化 INMP441 麦克风
 */
esp_err_t inmp441_init(inmp441_recorder_t *rec);

/**
 * @brief 开始录音，文件自动以 WAV 格式保存到 SD 卡
 */
esp_err_t inmp441_start_record(inmp441_recorder_t *rec, const char *filename);

/**
 * @brief 停止录音并关闭文件
 */
void inmp441_stop_record(inmp441_recorder_t *rec);

/**
 * @brief 录音任务
 */
void inmp441_record_task(void *param);

#ifdef __cplusplus
}
#endif

#endif // INMP441_H
