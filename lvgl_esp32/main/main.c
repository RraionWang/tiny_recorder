
#include "encoder_input.h"
#include "eez-flow.h"
#include "led_control.h"
#include "vars.h"
#include <stdint.h>
#include "actions.h"
#include "rc522_reader.h"
#include "sdcard.h"
#include "lcd.h"
#include "esp_err.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "speaker.h"
#include "recorder.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "time.h"
#include "esp_log.h"
#include "recorder.h"




// 测试代码开始

// === 按键定义 ===
// #define BUTTON_A_GPIO  GPIO_NUM_12
// #define BUTTON_B_GPIO  GPIO_NUM_13
// #define BUTTON_PRESSED 0  // 假设按键按下时为低电平

static const char* TAG = "main";













void tick_task(void *pvParam)
{
  

    while (1) {
              ui_tick() ;
        vTaskDelay(pdMS_TO_TICKS(100));  // 每隔 1 秒执行一次
    }
}



/* 主函数入口 */
void app_main(void)
{
 

    
     ESP_ERROR_CHECK(app_lcd_init());
     ESP_ERROR_CHECK(app_lvgl_init());


 

     ui_init() ; 


    sd_init();
    // sd_wr_test() ; 
    // ui_tick() ; 

     wav_player_init();
    // wav_player_play("/sdcard/canon.wav") ;

    rc522_reader_init() ; 







    // while(1){
      
    //   if(gpio_get_level(17) == false){
    //     set_var_is_detected_rfid_new_card(true);
    //       ESP_LOGI(TAG, "✅ 变成true: %d",get_var_is_detected_rfid_new_card());
    //   }else{
    //        set_var_is_detected_rfid_new_card(false);
    //          ESP_LOGI(TAG, "✅ 变成false:%d",get_var_is_detected_rfid_new_card());
    //   }

    //     ui_tick();
    //     vTaskDelay(pdMS_TO_TICKS(10));
    // }

    //    ESP_LOGI(TAG, "在写之前值:%d",get_var_is_detected_rfid_new_card());
    // set_var_is_detected_rfid_new_card(true);
    //     ESP_LOGI(TAG, "在写之后值:%d",get_var_is_detected_rfid_new_card());




    // vTaskDelay(pdMS_TO_TICKS(2000)); // 等待SD卡就绪
    // ESP_ERROR_CHECK(inmp441_start_record(&recorder, "rec_test.wav"));
    // vTaskDelay(pdMS_TO_TICKS(10000)); // 录10秒
    // inmp441_stop_record(&recorder);


      xTaskCreate(tick_task, "tixk_task", 2048, NULL, 4, NULL);


  
    
    
}


