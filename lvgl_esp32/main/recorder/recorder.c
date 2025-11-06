#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "recorder.h"
#include "driver/i2s_std.h"





#define I2S_BCLK_PIN   GPIO_NUM_4
#define I2S_WS_PIN     GPIO_NUM_5
#define I2S_DIN_PIN    GPIO_NUM_6
#define I2S_PORT       I2S_NUM_1

static i2s_chan_handle_t i2s_rx_handle = NULL;
static bool is_inited = false;

static const char* TAG = "recorder" ; 

bool inmp441_init(uint32_t sample_rate)
{
    if (is_inited) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    // 1. 创建 I2S RX 通道
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 4;
    chan_cfg.dma_frame_num = 128;
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL,&i2s_rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(ret));
        return false;
    }

    // 2. 使用宏初始化 slot_cfg（注意：只有 2 个参数！）
    i2s_std_slot_config_t slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_32BIT,
        I2S_SLOT_MODE_MONO
    );

    // 手动覆盖关键字段以适配 INMP441
    slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;  // 或 I2S_STD_SLOT_RIGHT，取决于 L/R 引脚接法
    slot_cfg.bit_shift = false;              // 关键：保持 24-bit 左对齐在 32-bit 中
    // 注意：v5.5 中没有 msb_first 字段，默认就是 MSB first（Philips 模式）

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DIN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(i2s_rx_handle, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(ret));
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
        return false;
    }

    ret = i2s_channel_enable(i2s_rx_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(ret));
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
        return false;
    }

    is_inited = true;
    ESP_LOGI(TAG, "INMP441 initialized at %lu Hz", sample_rate);
    return true;
}

size_t inmp441_read(int32_t *buffer, size_t num_samples, uint32_t timeout_ms)
{
    if (!is_inited || !buffer || num_samples == 0) {
        return 0;
    }

    size_t bytes_to_read = num_samples * sizeof(int32_t);
    size_t bytes_read = 0;

    esp_err_t ret = i2s_channel_read(i2s_rx_handle, buffer, bytes_to_read, &bytes_read, pdMS_TO_TICKS(timeout_ms));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return bytes_read / sizeof(int32_t);
}

void inmp441_deinit(void)
{
    if (!is_inited) return;

    if (i2s_rx_handle) {
        i2s_channel_disable(i2s_rx_handle);
        i2s_del_channel(i2s_rx_handle);
        i2s_rx_handle = NULL;
    }
    is_inited = false;
    ESP_LOGI(TAG, "INMP441 deinitialized");
}


#define RECORD_SAMPLE_RATE  (48000)
#define RECORD_BUFFER_SIZE  (1024)

// ❗ 不再使用固定路径，改为动态
static char current_record_file[256] = {0}; // 足够存 /sdcard/xxxxxxxxxxxx.wav

static TaskHandle_t record_task_handle = NULL;
static volatile bool is_recording = false;
static SemaphoreHandle_t record_mutex = NULL;

// WAV 头结构（保持不变）
typedef struct {
    char riff[4];
    uint32_t overall_size;
    char wave[4];
    char fmt_chunk[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data_chunk[4];
    uint32_t data_size;
} __attribute__((packed)) wav_header_t;


static void write_initial_wav_header(FILE *fp, uint32_t sample_rate, uint32_t bits_per_sample)
{
    wav_header_t header = {0};
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt_chunk, "fmt ", 4);
    memcpy(header.data_chunk, "data", 4);

    uint32_t byte_rate = sample_rate * 1 * (bits_per_sample / 8);
    uint16_t block_align = 1 * (bits_per_sample / 8);

    header.fmt_size = 16;
    header.audio_format = 1;          // PCM
    header.num_channels = 1;
    header.sample_rate = sample_rate;
    header.byte_rate = byte_rate;
    header.block_align = block_align;
    header.bits_per_sample = bits_per_sample;

    // 关键：data_size 设为 0xFFFFFFFF 表示“未知长度”
    header.data_size = 0xFFFFFFFF;
    header.overall_size = 0xFFFFFFFF; // 同样设为最大值

    fwrite(&header, 1, sizeof(header), fp);
    fflush(fp); // 确保写入
}


// 录音任务：使用 current_record_file
static void record_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Recording task started, file: %s", current_record_file);

    FILE *fp = fopen(current_record_file, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", current_record_file);
        is_recording = false;
        vTaskDelete(NULL);
        return;
    }

    // ✅ 写入有效的初始 header（data_size = 0xFFFFFFFF）
    write_initial_wav_header(fp, RECORD_SAMPLE_RATE, 32);

    int32_t *buffer = malloc(RECORD_BUFFER_SIZE * sizeof(int32_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate buffer");
        fclose(fp);
        is_recording = false;
        vTaskDelete(NULL);
        return;
    }

    uint32_t total_samples = 0;
    const uint32_t timeout_ms = 100;

    while (is_recording) {
        size_t samples_read = inmp441_read(buffer, RECORD_BUFFER_SIZE, timeout_ms);
        if (samples_read > 0) {
            size_t bytes_written = fwrite(buffer, sizeof(int32_t), samples_read, fp);
            if (bytes_written != samples_read) {
                ESP_LOGW(TAG, "File write mismatch");
            }
            total_samples += samples_read;
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    free(buffer);
    fclose(fp); // 先关闭文件

    // （可选）尝试修正文件头 —— 但 FAT32 不支持，所以跳过
    // 如果你坚持要修正，需重新打开 r+ 模式，但风险高，不推荐

    ESP_LOGI(TAG, "Recording saved. Total samples: %lu, file: %s",
             (unsigned long)total_samples, current_record_file);

    memset(current_record_file, 0, sizeof(current_record_file));
    vTaskDelete(NULL);
}


// 【新增】按指定路径开始录音
esp_err_t start_recording_to_file(const char *filepath)
{
    if (!filepath || strlen(filepath) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(filepath) >= sizeof(current_record_file)) {
        ESP_LOGE(TAG, "File path too long");
        return ESP_ERR_NO_MEM;
    }

    if (is_recording) {
        ESP_LOGW(TAG, "Already recording, stop first");
        return ESP_ERR_INVALID_STATE;
    }

    // 创建 mutex（懒加载）
    if (!record_mutex) {
        record_mutex = xSemaphoreCreateMutex();
        if (!record_mutex) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_FAIL;
        }
    }

    if (xSemaphoreTake(record_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    strncpy(current_record_file, filepath, sizeof(current_record_file) - 1);

    if (!inmp441_init(RECORD_SAMPLE_RATE)) {
        ESP_LOGE(TAG, "Failed to initialize INMP441");
        xSemaphoreGive(record_mutex);
        return ESP_FAIL;
    }

    is_recording = true;
    xSemaphoreGive(record_mutex);

    BaseType_t ret = xTaskCreate(
        record_task,
        "record_task",
        4096,
        NULL,
        tskIDLE_PRIORITY + 2,
        &record_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create recording task");
        inmp441_deinit();
        is_recording = false;
        memset(current_record_file, 0, sizeof(current_record_file));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Recording started to: %s", filepath);
    return ESP_OK;
}

// 保留原有接口（可选）
esp_err_t start_recording(void)
{
    return start_recording_to_file("/sdcard/record.wav");
}

// 停止录音（通用）
esp_err_t stop_recording(void)
{
    if (!is_recording) {
        return ESP_ERR_INVALID_STATE;
    }

    is_recording = false;

    if (record_task_handle) {
        // 等待任务结束（最多 2 秒）
        for (int i = 0; i < 20; i++) {
            eTaskState state = eTaskGetState(record_task_handle);
            if (state == eDeleted || state == eInvalid) break;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        record_task_handle = NULL;
    }

    inmp441_deinit();
    ESP_LOGI(TAG, "Recording stopped");
    return ESP_OK;
}

// 【新增】查询是否正在录音
bool is_recording_active(void)
{
    return is_recording;
}


