#ifndef SPEAKER_H
#define SPEAKER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化音频播放系统（SD卡 + I2S）
 * @return true 成功，false 失败
 */
bool wav_player_init(void);

/**
 * @brief 播放指定路径的 WAV 文件（仅支持 16-bit PCM）
 * @param path 文件路径，如 "/sdcard/test.wav"
 */
void wav_player_play(const char *path);

#ifdef __cplusplus
}
#endif

#endif // WAV_PLAYER_H