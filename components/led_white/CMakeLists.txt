idf_component_register(SRCS "led_white.c"
                    INCLUDE_DIRS "include"
                    REQUIRES ble_foco
                            led_controller)
