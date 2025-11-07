#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_start_record(lv_event_t * e);
extern void action_stop_record(lv_event_t * e);
extern void action_show_sd_card_list(lv_event_t * e);
extern void action_drop_record_file(lv_event_t * e);
void ui_set_sd_list(lv_obj_t *list);
lv_obj_t *ui_get_sd_list(void);
static void file_button_event_cb(lv_event_t *e)  ; 

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/