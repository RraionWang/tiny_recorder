#include "actions.h"
#include "recorder.h"
#include "string.h"
#include "malloc.h"
#include "esp_log.h"
#include "recorder_control.h"


static inmp441_recorder_t recorder;

const char* TAG = "action";

char rfid_uid[100] = { 0 };

const char *get_var_rfid_uid() {
    return rfid_uid;
}

void set_var_rfid_uid(const char *value) {
    strncpy(rfid_uid, value, sizeof(rfid_uid) / sizeof(char));
    rfid_uid[sizeof(rfid_uid) / sizeof(char) - 1] = 0;
}







void action_start_record(lv_event_t *e) {
   
    
    if (!recorder_is_running()) {
            // 开始录音
            ESP_LOGI(TAG, "Record button clicked - start recording");
            recorder_start("test.wav");


        } else {
            ESP_LOGW(TAG, "Already recording");
        }


}


void action_stop_record(lv_event_t *e) {

    

    if (recorder_is_running()) {
            ESP_LOGI(TAG, "Stop button clicked - stop recording");
            recorder_stop();

        } else {
            ESP_LOGW(TAG, "No recording to stop");
        }


}




void action_show_sd_card_list(lv_event_t *e) {
    // TODO: Implement action show_sd_card_list here
}


void action_drop_record_file(lv_event_t *e) {
    // TODO: Implement action drop_record_file here
}

 void action_test(lv_event_t * e){

}