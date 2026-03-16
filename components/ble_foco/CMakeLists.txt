idf_component_register(SRCS "ble_foco.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bt driver nvs_flash  # ← AÑADE ESTAS DEPENDENCIAS
                    PRIV_REQUIRES "")
