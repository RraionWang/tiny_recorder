#ifndef RECORDER_H
#define RECORDER_H

#include "esp_err.h"
#include "driver/i2s_std.h"
#include <stdio.h>
#include <stdbool.h>
#include "pin_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------
// 配置参数（可按需修改）
//--------------------------------------------------------
#define I2S_PORT            I2S_NUM_0
#define SAMPLE_RATE_HZ      48000
#define SAMPLE_BITS         I2S_DATA_BIT_WIDTH_32BIT
#define CHANNEL_MODE          I2S_SLOT_MODE_MONO
#define BUFFER_SIZE         1024



//--------------------------------------------------------
// 录音器结构体定义
//--------------------------------------------------------
typedef struct {
    i2s_chan_handle_t rx_chan;   // I2S 接收通道句柄
    FILE *file;                  // 当前 WAV 文件
    bool is_recording;           // 是否正在录音
    char filepath[128];          // 文件路径
} inmp441_recorder_t;

//--------------------------------------------------------
// 函数声明
//--------------------------------------------------------

// 初始化 I2S + 麦克风通道
esp_err_t inmp441_init(inmp441_recorder_t *rec);

// 启动录音任务（异步）
esp_err_t inmp441_start_record(inmp441_recorder_t *rec, const char *filename);

// 停止录音任务并保存文件
void inmp441_stop_record(inmp441_recorder_t *rec);

#ifdef __cplusplus
}
#endif

#endif /* RECORDER_H */
