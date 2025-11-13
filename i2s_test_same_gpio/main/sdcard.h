#ifndef SDCARD_H
#define SDCARD_H

#include "esp_err.h"

#define MOUNT_POINT "/sdcard"
void sd_init();

// 读取文件
 esp_err_t read_file(const char *path);

// 写文件
 esp_err_t write_file(const char *path, const char *data);


// 仅仅作作为测试使用
void sd_wr_test(void) ; 



#endif