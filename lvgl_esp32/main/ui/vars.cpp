#include "vars.h"

bool is_detected_rfid_new_card;

bool get_var_is_detected_rfid_new_card() {
    return is_detected_rfid_new_card;
}

void set_var_is_detected_rfid_new_card(bool value) {
    is_detected_rfid_new_card = value;
}
