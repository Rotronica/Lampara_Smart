#ifndef BLE_FOCO_H
#define BLE_FOCO_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callbacks que tu aplicación puede registrar para recibir eventos BLE
 */
typedef struct {
    void (*on_color_change)(uint8_t r, uint8_t g, uint8_t b);
    void (*on_brightness_change)(uint8_t brightness);
    void (*on_mode_change)(uint8_t mode);
    void (*on_connect)(void);
    void (*on_disconnect)(void);
    void (*on_white_change)(uint16_t kelvin);
} ble_foco_callbacks_t;

/**
 * @brief Inicializa el servicio BLE del foco
 * 
 * Esta función inicializa todo el stack BLE, registra los callbacks,
 * configura el advertising y crea la tabla GATT.
 * 
 * @return ESP_OK si éxito, otro valor si error
 */
esp_err_t ble_foco_init(void);

/**
 * @brief Registra callbacks para eventos del foco
 * 
 * @param cbs Estructura con los callbacks (puede ser NULL para los que no interesen)
 */
void ble_foco_register_callbacks(const ble_foco_callbacks_t *cbs);

/**
 * @brief Actualiza el valor de color en el servidor GATT (si cambia internamente)
 * 
 * @param r Rojo (0-255)
 * @param g Verde (0-255)
 * @param b Azul (0-255)
 * @return ESP_OK si éxito
 */
esp_err_t ble_foco_update_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Actualiza el valor de brillo en el servidor GATT
 * 
 * @param brightness Brillo (0-100)
 * @return ESP_OK si éxito
 */
esp_err_t ble_foco_update_brightness(uint8_t brightness);

/**
 * @brief Actualiza el valor de modo en el servidor GATT
 * 
 * @param mode Modo (0=sólido, 1=efecto, etc.)
 * @return ESP_OK si éxito
 */
esp_err_t ble_foco_update_mode(uint8_t mode);

/**
 * @brief Obtiene los valores actuales del foco
 */
void ble_foco_get_current(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *brightness, uint8_t *mode);

/**
 * @brief Actualiza el valor de temperatura de blanco en el servidor GATT
 * @param kelvin Temperatura en Kelvin (2700-6500)
 * @return ESP_OK si éxito
 */
esp_err_t ble_foco_update_white(uint16_t kelvin);

#ifdef __cplusplus
}
#endif

#endif // BLE_FOCO_H