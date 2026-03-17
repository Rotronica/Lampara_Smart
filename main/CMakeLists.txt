idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ble_foco 
                            led_controller 
                            led_color  # ← DEPENDENCIAS DE TU CÓDIGO
                    )
