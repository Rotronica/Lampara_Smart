#ifndef LED_WHITE_H
#define LED_WHITE_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Temperatura de color mínima y máxima en Kelvin
 */
#define WHITE_MIN_KELVIN    2700  // Cálido (ámbar)
#define WHITE_MAX_KELVIN    6500  // Frío (azul)

/**
 * @brief Inicializa el módulo de blanco
 */
esp_err_t led_white_init(void);

/**
 * @brief Establece la temperatura de color para todos los LEDs
 * @param kelvin Temperatura en Kelvin (2700-6500)
 */
void led_white_set_temperature(uint16_t kelvin);

/**
 * @brief Obtiene la temperatura actual
 */
uint16_t led_white_get_temperature(void);

/**
 * @brief Convierte temperatura Kelvin a valores RGB aproximados
 * @param kelvin Temperatura en Kelvin
 * @param r, g, b Punteros para almacenar los valores RGB
 */
void led_white_kelvin_to_rgb(uint16_t kelvin, uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif

#endif // LED_WHITE_H