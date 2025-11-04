#include "actions.h"
#include "led_control.h"
#include "vars.h"

extern int32_t my_value;

void action_led_on(lv_event_t *e) {
    // TODO: Implement action led_on here
    led_toggle() ; 
}



void action_set_var(lv_event_t *e) {
    // TODO: Implement action set_var here

 
}



