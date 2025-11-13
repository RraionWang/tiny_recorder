#include "audio.h"
#include "recorder.h"
#include "speaker.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdcard.h"
void app_main(void)
{
    sd_init() ;

//     audio_i2s_init();

//     // === 录音 ===
//     recorder_record_wav("/sdcard/test.wav", 5);

//     // === 播放 ===
//     wav_player_play("/sdcard/test.wav");
 }
