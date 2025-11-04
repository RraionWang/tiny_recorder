#include <string.h>
#include "vars.h"

char read_val[100] = { 0 };

const char *get_var_read_val() {
    return read_val;
}

void set_var_read_val(const char *value) {
    strncpy(read_val, value, sizeof(read_val) / sizeof(char));
    read_val[sizeof(read_val) / sizeof(char) - 1] = 0;
}
