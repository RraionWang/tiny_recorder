idf_component_register(SRCS "main.c" 
                            "encoder_input.c"
                            "ui/idf_flow/src/ui/eez-flow.cpp"
                            "ui/idf_flow/src/ui/screens.c"
                            "ui/idf_flow/src/ui/styles.c"
                            "ui/idf_flow/src/ui/images.c"
                            "ui/idf_flow/src/ui/ui.c"
                            "ui/idf_flow/src/ui/action.c"
                            "led/led_control.c"
                            "ui/idf_flow/src/ui/vars.cpp"
                            
                    INCLUDE_DIRS "."
                                 "ui/idf_flow/src/ui"
                                 "led")