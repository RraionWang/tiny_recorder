#ifndef RECORDER_CONTROL_H
#define RECORDER_CONTROL_H

#include "esp_err.h"
#include "recorder.h"   // 包含 inmp441_recorder_t 定义

#ifdef __cplusplus
extern "C" {
#endif

// 启动录音
void recorder_start(const char* filename) ;

// 停止录音
void recorder_stop(void);

// 查询录音是否正在进行
bool recorder_is_running(void);

// （可选）彻底释放 I2S 通道资源
void recorder_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* RECORDER_CONTROL_H */
