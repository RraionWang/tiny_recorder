#include "recorder_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <time.h>
#include "esp_check.h"

static const char *TAG = "RECORDER_CTRL";

// 全局录音实例（模块级单例）
static inmp441_recorder_t s_recorder = {0};

// 状态标志
static bool s_is_running = false;
static bool s_is_initialized = false;  // 只初始化一次 I2S


//--------------------------------------------------------
// 启动录音
//--------------------------------------------------------
void recorder_start(const char* filename)
{
    if (s_is_running) {
        ESP_LOGW(TAG, "Recorder already running");
        return;
    }

    // ✅ 初始化一次
    if (!s_is_initialized) {
        ESP_LOGI(TAG, "Initializing INMP441...");
        esp_err_t err = inmp441_init(&s_recorder);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init INMP441 (%s)", esp_err_to_name(err));
            return;
        }
        s_is_initialized = true;
    }

    // ✅ 自动生成文件名（防止覆盖）
    

    // ✅ 启动录音任务
    ESP_LOGI(TAG, "Starting record -> %s", filename);
    esp_err_t ret = inmp441_start_record(&s_recorder, filename);
    if (ret == ESP_OK) {
        s_is_running = true;
    } else {
        ESP_LOGE(TAG, "Failed to start record (%s)", esp_err_to_name(ret));
    }
}

//--------------------------------------------------------
// 停止录音
//--------------------------------------------------------
void recorder_stop(void)
{
    if (!s_is_running) {
        ESP_LOGW(TAG, "Recorder not running");
        return;
    }

    ESP_LOGI(TAG, "Stopping record...");
    inmp441_stop_record(&s_recorder);
    s_is_running = false;

    // 等待缓冲区写完
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Record stopped and file saved");

    // ⚙️ 可选：是否释放 I2S 资源
    // i2s_del_channel(s_recorder.rx_chan);
    // s_recorder.rx_chan = NULL;
    // s_is_initialized = false;
}

//--------------------------------------------------------
// 查询录音状态
//--------------------------------------------------------
bool recorder_is_running(void)
{
    return s_is_running;
}

//--------------------------------------------------------
// 手动释放资源（如果需要重新配置采样率等）
//--------------------------------------------------------
void recorder_deinit(void)
{
    if (s_recorder.rx_chan) {
        i2s_del_channel(s_recorder.rx_chan);
        s_recorder.rx_chan = NULL;
    }
    s_is_initialized = false;
    s_is_running = false;
    ESP_LOGI(TAG, "Recorder deinitialized");
}
