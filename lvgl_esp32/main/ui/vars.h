#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_SD_FILE_LIST = 0
};

// Native global variables

extern bool get_var_is_detected_rfid_new_card();
extern void set_var_is_detected_rfid_new_card(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/