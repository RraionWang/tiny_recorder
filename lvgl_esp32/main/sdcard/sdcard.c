#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "pin_cng.h"
#define TAG "SDMMC_TEST"
#define MOUNT_POINT "/sdcard"

static esp_err_t write_file(const char *path, const char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGI(TAG, "File written");
    return ESP_OK;
}

static esp_err_t read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[128];
    fgets(line, sizeof(line), f);
    fclose(f);
    ESP_LOGI(TAG, "Read from file: '%s'", line);
    return ESP_OK;
}

sdmmc_card_t *card;


void sd_init(){
 esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SD card (SDMMC mode)");

    // SDMMC 主机配置
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // 20 MHz 默认频率

    // SDMMC 槽配置
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4; // 若只用 D0 改为 1
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP; // 启用内部上拉

#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = SD_CLK;
    slot_config.cmd = SD_CMD;
    slot_config.d0  = SD_D0;
    slot_config.d1  = SD_D1;
    slot_config.d2  = SD_D2;
    slot_config.d3  = SD_D3;
#endif

    // 挂载 FAT 文件系统
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };


    ESP_LOGI(TAG, "Mounting filesystem...");
    ret = esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Mount failed. Try enabling format_if_mount_failed.");
        } else {
            ESP_LOGE(TAG, "Card init failed. Check wiring or pull-ups.");
        }
        return;
    }

    ESP_LOGI(TAG, "Filesystem mounted successfully");
    sdmmc_card_print_info(stdout, card);

}
void sd_wr_test(void)
{
    esp_err_t ret;

    // 写入与读取文件测试
    const char *path = MOUNT_POINT "/test.txt";
    ret = write_file(path, "Hello ESP32S3 SDMMC!");
    if (ret == ESP_OK) {
        read_file(path);
    }

    // 卸载
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    ESP_LOGI(TAG, "Card unmounted, example complete.");
}
