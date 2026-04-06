#ifndef WS2812_DRIVER_H
#define WS2812_DRIVER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Configuración del driver WS2812
     */
    typedef struct
    {
        int gpio_pin;           //!< GPIO para la línea de datos
        int num_leds;           //!< Número de LEDs en la tira
        uint32_t resolution_hz; //!< Resolución RMT (10MHz recomendado)
    } ws2812_driver_config_t;

    /**
     * @brief Inicializa el driver WS2812
     */
    esp_err_t ws2812_driver_init(const ws2812_driver_config_t *config);

    /**
     * @brief Establece el color de un LED (formato GRB, ya con brillo aplicado)
     */
    void ws2812_driver_set_color(int index, uint8_t g, uint8_t r, uint8_t b);

    /**
     * @brief Envía los datos a los LEDs
     */
    void ws2812_driver_update(void);

    /**
     * @brief Limpia el buffer interno (todos los LEDs apagados)
     */
    void ws2812_driver_clear(void);

    /**
     * @brief Obtiene el número de LEDs configurados
     */
    int ws2812_driver_get_count(void);

    /**
     * @brief Libera recursos del driver
     */
    void ws2812_driver_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WS2812_DRIVER_H