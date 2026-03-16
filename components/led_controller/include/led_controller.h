#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuración del controlador de LEDs
 */
typedef struct {
    int gpio_pin;           //!< GPIO donde está conectada la tira de LEDs
    int num_leds;           //!< Número total de LEDs en la tira
    uint32_t resolution_hz; //!< Resolución del RMT (10MHz recomendado para WS2812)
} led_controller_config_t;

/**
 * @brief Inicializa el controlador de LEDs
 * 
 * @param config Configuración del controlador
 * @return ESP_OK si éxito, otro valor si error
 */
esp_err_t led_controller_init(const led_controller_config_t *config);

/**
 * @brief Establece el color de un LED específico (sin actualizar la tira)
 * 
 * @param index Índice del LED (0 basado)
 * @param r Rojo (0-255)
 * @param g Verde (0-255)
 * @param b Azul (0-255)
 * @return ESP_OK si éxito, ESP_ERR_INVALID_ARG si índice inválido
 */
esp_err_t led_controller_set_color(int index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Envía todos los colores almacenados a la tira de LEDs
 * 
 * @return ESP_OK si éxito, ESP_ERR_INVALID_STATE si no inicializado
 */
esp_err_t led_controller_update(void);

/**
 * @brief Establece el brillo global (0-100)
 * 
 * @param brightness Brillo (0-100)
 * @return ESP_OK si éxito
 */
esp_err_t led_controller_set_brightness(uint8_t brightness);

/**
 * @brief Apaga todos los LEDs
 * 
 * @return ESP_OK si éxito
 */
esp_err_t led_controller_clear(void);

/**
 * @brief Obtiene el color actual de un LED
 * 
 * @param index Índice del LED
 * @param r Puntero para almacenar rojo (puede ser NULL)
 * @param g Puntero para almacenar verde (puede ser NULL)
 * @param b Puntero para almacenar azul (puede ser NULL)
 * @return ESP_OK si éxito
 */
esp_err_t led_controller_get_color(int index, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @brief Convierte HSV a RGB (función utilitaria)
 * 
 * @param h Hue (0-359)
 * @param s Saturation (0-100)
 * @param v Value (0-100)
 * @param r Puntero para rojo resultante
 * @param g Puntero para verde resultante
 * @param b Puntero para azul resultante
 */
void led_controller_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif

#endif // LED_CONTROLLER_H