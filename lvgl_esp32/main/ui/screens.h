#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *detected_rfid_page;
    lv_obj_t *recording_page;
    lv_obj_t *stop_recording_page;
    lv_obj_t *sdcard_file_page;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_DETECTED_RFID_PAGE = 2,
    SCREEN_ID_RECORDING_PAGE = 3,
    SCREEN_ID_STOP_RECORDING_PAGE = 4,
    SCREEN_ID_SDCARD_FILE_PAGE = 5,
};

void create_screen_main();
void tick_screen_main();

void create_screen_detected_rfid_page();
void tick_screen_detected_rfid_page();

void create_screen_recording_page();
void tick_screen_recording_page();

void create_screen_stop_recording_page();
void tick_screen_stop_recording_page();

void create_screen_sdcard_file_page();
void tick_screen_sdcard_file_page();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/