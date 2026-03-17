idf_component_register(SRCS "led_white.c"
                    INCLUDE_DIRS "include"
                    REQUIRES    led_controller
                                ble_foco)
