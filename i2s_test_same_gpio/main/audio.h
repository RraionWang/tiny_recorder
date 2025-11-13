// audio.h
#ifndef AUDIO_H
#define AUDIO_H

#include "driver/i2s_std.h"

#ifdef __cplusplus
extern "C" {
#endif

extern i2s_chan_handle_t tx_chan;
extern i2s_chan_handle_t rx_chan;

void audio_i2s_init(void);

#ifdef __cplusplus
}
#endif

#endif
