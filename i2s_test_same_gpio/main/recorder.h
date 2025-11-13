#ifndef RECORDER_H
#define RECORDER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void recorder_record_wav(const char *path, int seconds);

#ifdef __cplusplus
}
#endif

#endif
