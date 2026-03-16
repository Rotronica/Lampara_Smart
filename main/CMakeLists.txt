idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ble_foco led_controller  # ← DEPENDENCIAS DE TU CÓDIGO
                    )
