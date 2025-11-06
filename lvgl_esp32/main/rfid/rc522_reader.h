#ifndef RC522_READER_H
#define RC522_READER_H

#include "esp_err.h"
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"

#ifdef __cplusplus
extern "C" {
#endif





// 初始化 RC522 模块
void rc522_reader_init(void);

// 启动卡片扫描
esp_err_t rc522_reader_start(void);

// 停止卡片扫描
void rc522_reader_stop(void);

// 获取当前检测到的卡片 UID（16进制字符串），若无卡返回 NULL
const char *rc522_reader_get_uid(void);

#ifdef __cplusplus
}
#endif

#endif // RC522_READER_H
