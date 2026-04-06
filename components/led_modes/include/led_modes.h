#ifndef LED_MODES_H
#define LED_MODES_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Modos de operación del foco
     */
    typedef enum
    {
        LED_MODE_SOLID = 0,   // Color sólido (gestionado externamente)
        LED_MODE_RAINBOW = 1, // Arcoíris cíclico
        LED_MODE_SUNSET = 2,  // Simulación de atardecer
        LED_MODE_PARTY = 3,   // Colores aleatorios rápidos
        LED_MODE_RELAX = 4,   // Transiciones suaves
        LED_MODE_NIGHT = 5,   // Luz cálida tenue
        LED_MODE_STORM = 6,   // Efecto de relámpagos
        LED_MODE_MAX
    } led_mode_t;

    /**
     * @brief Configuración para cada modo
     */
    typedef struct
    {
        uint8_t speed;      // Velocidad del efecto (0-100)
        uint8_t brightness; // Brillo base para el efecto
        uint8_t color_r;    // Color base (para modos que lo usen)
        uint8_t color_g;
        uint8_t color_b;
        uint16_t kelvin; // Temperatura base (para modos blancos)
    } led_mode_config_t;

    /**
     * @brief Inicializa el módulo de modos
     */
    esp_err_t led_modes_init(void);

    /**
     * @brief Inicia un modo específico
     * @param mode Modo a ejecutar
     * @param config Configuración del modo (puede ser NULL para usar valores por defecto)
     */
    esp_err_t led_modes_start(led_mode_t mode, const led_mode_config_t *config);

    /**
     * @brief Detiene el modo actual
     */
    esp_err_t led_modes_stop(void);

    /**
     * @brief Obtiene el modo actualmente activo
     * @return Modo actual, o LED_MODE_SOLID si ninguno
     */
    led_mode_t led_modes_get_current(void);

    /**
     * @brief Actualiza parámetros del modo actual (velocidad, brillo, etc.)
     */
    esp_err_t led_modes_update_config(const led_mode_config_t *config);

    /**
     * @brief Verifica si un modo está activo
     * @return true si hay un modo activo, false si está en modo sólido
     */
    bool led_modes_is_active(void);

#ifdef __cplusplus
}
#endif

#endif // LED_MODES_H