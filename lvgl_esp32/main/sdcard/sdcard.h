#ifndef SDCARD_H
#define SDCARD_H

void sd_init();
void sd_wr_test(void) ; 
void sd_list_wav_files(void (*callback)(const char *filename)) ;


#endif