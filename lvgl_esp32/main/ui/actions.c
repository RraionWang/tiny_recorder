#include "actions.h"
#include "recorder.h"
#include "string.h"
#include "malloc.h"
#include "esp_log.h"
#include "recorder_control.h"
#include "sdcard.h"
#include "lvgl.h"
#include "esp_vfs.h"
#include "speaker.h"


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

static void add_file_to_list(const char *filename) {
    lv_obj_t *list = ui_get_sd_list();
    if (!list) return;

    // 1️⃣ 添加按钮
    lv_obj_t *btn = lv_list_add_btn(list, NULL, filename);

    // 2️⃣ 注册点击回调，并把文件名作为 user_data 传入
    lv_obj_add_event_cb(btn, file_button_event_cb, LV_EVENT_CLICKED, (void *)strdup(filename));

    // 3️⃣ 样式优化
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xF5F5F5), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(btn, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btn, 4, LV_PART_MAIN | LV_STATE_DEFAULT);

    ESP_LOGI("SD_LIST", "添加文件: %s", filename);
}




// 🎵 扫描 SD 卡文件并填充列表
void action_show_sd_card_list(lv_event_t *e) {
    lv_obj_t *list = ui_get_sd_list();  // 获取 LVGL 列表对象

    if (!list) {
        ESP_LOGE("action", "找不到 SD 列表对象！");
        return;
    }

    lv_obj_clean(list);  // 清空旧内容
    lv_list_add_text(list, "Record List");

    const char *path = "/sdcard";
    DIR *dir = opendir(path);
    if (!dir) {
        ESP_LOGE("action", "无法打开目录 %s", path);
        lv_list_add_text(list, "⚠️ 无法读取 SD 卡");
        return;
    }

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;
        const char *ext = strrchr(entry->d_name, '.');
        if (ext && strcasecmp(ext, ".wav") == 0) {
            add_file_to_list(entry->d_name);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        lv_list_add_text(list, "（未找到任何 .wav 文件）");
    }

    ESP_LOGI("action", "已将 %d 个 WAV 文件加载到列表中", count);
}

void action_drop_record_file(lv_event_t *e) {
    // TODO: Implement action drop_record_file here
}

 void action_test(lv_event_t * e){

}



// 创建获取表格函数
static lv_obj_t *g_sd_list = NULL;

void ui_set_sd_list(lv_obj_t *list) {
    g_sd_list = list;
}

lv_obj_t *ui_get_sd_list(void) {
    return g_sd_list;
}

//文件按钮回调
static void file_button_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *fname = (const char *)lv_event_get_user_data(e);

    if (!fname) {
        ESP_LOGW("SD_LIST", "⚠️ 文件名为空，无法播放");
        return;
    }

    // 拼接完整路径
    char fullpath[128];
    snprintf(fullpath, sizeof(fullpath), "/sdcard/%s", fname);

    ESP_LOGI("SD_LIST", "▶️ 播放文件: %s", fullpath);

    // ✅ 调用你的播放函数
    wav_player_play(fullpath);

    // （可选）点击时按钮高亮
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xA0D8FF), LV_PART_MAIN | LV_STATE_DEFAULT);
}
