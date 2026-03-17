#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración del controlador base de LEDs
 */
typedef struct {
    int gpio_pin;           //!< GPIO donde está conectada la tira de LEDs
    int num_leds;           //!< Número total de LEDs en la tira
    uint32_t resolution_hz; //!< Resolución del RMT (10MHz recomendado)
} led_controller_config_t;

/**
 * @brief Inicializa el controlador base de LEDs
 */
esp_err_t led_controller_init(const led_controller_config_t *config);

/**
 * @brief Establece el color de un LED específico (sin actualizar)
 */
esp_err_t led_controller_set_color(int index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Envía todos los colores almacenados a la tira de LEDs
 */
esp_err_t led_controller_update(void);

/**
 * @brief Establece el brillo global (0-100)
 */
esp_err_t led_controller_set_brightness(uint8_t brightness);

/**
 * @brief Obtiene el brillo global actual
 */
uint8_t led_controller_get_brightness(void);

/**
 * @brief Apaga todos los LEDs
 */
esp_err_t led_controller_clear(void);

/**
 * @brief Obtiene el color actual de un LED
 */
esp_err_t led_controller_get_color(int index, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Devuelve el número de LEDs configurados
 */
int led_controller_get_count(void);

/**
 * @brief Convierte HSV a RGB (función utilitaria)
 */
void led_controller_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif

#endif // LED_CONTROLLER_H