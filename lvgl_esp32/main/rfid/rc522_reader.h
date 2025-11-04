// rc522_reader.h
#ifndef RC522_READER_H
#define RC522_READER_H

#include <stdbool.h>

/**
 * @brief 初始化 RC522 读卡器（后台自动轮询）
 */
void rc522_reader_init(void);

/**
 * @brief 获取当前卡片状态
 * @return true 表示有卡，false 表示无卡
 */
bool rc522_get_card_status(void);
bool rc522_get_latest_uid(uint8_t *uid, uint8_t *uid_len);

#endif // RC522_READER_H