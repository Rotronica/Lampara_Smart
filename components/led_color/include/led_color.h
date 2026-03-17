#ifndef LED_COLOR_H
#define LED_COLOR_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el módulo de color (usa led_controller internamente)
 */
esp_err_t led_color_init(void);

/**
 * @brief Establece el color RGB para todos los LEDs
 */
void led_color_set_all(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Establece el color RGB para un LED específico
 */
void led_color_set_led(int index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Obtiene el último color establecido (del primer LED)
 */
void led_color_get_current(uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Ejecuta un paso del efecto arcoíris
 */
void led_color_rainbow_step(void);

/**
 * @brief Envía los cambios al hardware
 */
void led_color_update(void);

#ifdef __cplusplus
}
#endif

#endif // LED_COLOR_H