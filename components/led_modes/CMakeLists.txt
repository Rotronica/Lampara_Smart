idf_component_register(SRCS "led_modes.c"
                    INCLUDE_DIRS "include"
                    REQUIRES led_controller 
                            led_color 
                            led_white)
