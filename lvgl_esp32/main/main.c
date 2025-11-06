
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





/* 主函数入口 */
void app_main(void)
{
 

    
    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());


 

    ui_init() ; 


    sd_init();
    sd_wr_test() ; 
    ui_tick() ; 

    wav_player_init();
    wav_player_play("/sdcard/canon.wav") ;

      rc522_reader_init();



    while(1){
        ui_tick();
        vTaskDelay(pdMS_TO_TICKS(10));
    }



    

    
    
}
