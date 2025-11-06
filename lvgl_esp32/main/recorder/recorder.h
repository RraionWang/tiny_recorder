#ifndef RECORDER_H
#define RECORDER_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t start_recording(void);                      // 默认路径
esp_err_t start_recording_to_file(const char *filepath); // 指定路径
esp_err_t stop_recording(void);
bool is_recording_active(void);

void inmp441_deinit(void);
size_t inmp441_read(int32_t *buffer, size_t num_samples, uint32_t timeout_ms);
void inmp441_deinit(void);


#ifdef __cplusplus
}
#endif

#endif // RECORDER_H