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
    FLOW_GLOBAL_VARIABLE_SD_FILE_LIST = 0,
    FLOW_GLOBAL_VARIABLE_FILE_LIST_OBJ = 1
};

// Native global variables

extern const char *get_var_rfid_uid();
extern void set_var_rfid_uid(const char *value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/