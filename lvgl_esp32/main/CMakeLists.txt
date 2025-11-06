idf_component_register(SRCS "main.c" 
                            "encoder_input.c"
                            "ui/eez-flow.cpp"
                            "ui/screens.c"
                            "ui/styles.c"
                            "ui/images.c"
                            "ui/ui.c"
                            "led/led_control.c"
                            "ui/ui_font_chinese_14.c"
                            "ui/ui_font_chinese_20.c"
                      
                             "rfid/rc522_reader.c"
                         
                             "sdcard/sdcard.c"
                             "lcd/lcd.c"
                             "speaker/speaker.c"
                             "recorder/recorder.c"
                             "ui/actions.cpp"
                             "ui/vars.cpp"

                            
                    INCLUDE_DIRS "."
                                 "sdcard"
                                 "ui"
                                 "led"
                                 "rfid"
                                 "lcd"
                                 "speaker"
                                 "recorder")