// inmp441.h
#ifndef RECORDER_H
#define RECORDER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 INMP441 麦克风（ESP32-S3 I2S RX）
 *
 * @param sample_rate 采样率（如 16000, 44100, 48000）
 * @return true 成功，false 失败
 */
bool inmp441_init(uint32_t sample_rate);

/**
 * @brief 读取音频数据（单声道，32-bit 左对齐）
 *
 * @param buffer 输出缓冲区（建议类型：int32_t*）
 * @param num_samples 要读取的样本数量（每个样本 4 字节）
 * @param timeout_ms 超时时间（毫秒）
 * @return 实际读取的样本数
 */
size_t inmp441_read(int32_t *buffer, size_t num_samples, uint32_t timeout_ms);

/**
 * @brief 停止并释放 I2S 资源
 */
void inmp441_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // INMP441_H