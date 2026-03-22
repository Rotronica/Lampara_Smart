#include "led_modes.h"
#include "led_controller.h"
#include "led_color.h"
#include "led_white.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "LED_MODES";

// Variables estáticas del módulo
static TaskHandle_t mode_task_handle = NULL;
static led_mode_t current_mode = LED_MODE_SOLID;
static led_mode_config_t current_config;
static bool mode_active = false;

// ==================== FUNCIONES PRIVADAS ====================

/**
 * @brief Modo ARCOÍRIS - Transición de colores cíclica
 */
static void mode_rainbow_task(void *pv)
{
    led_mode_config_t *config = (led_mode_config_t *)pv;
    uint16_t hue = 0;
    uint8_t r, g, b;
    
    ESP_LOGI(TAG, "🌈 Modo Arcoíris iniciado (velocidad: %d)", config->speed);
    
    while (1) {
        // Convertir HSV a RGB
        led_controller_hsv2rgb(hue, 100, config->brightness, &r, &g, &b);
        
        // Aplicar a todos los LEDs
        for (int i = 0; i < led_controller_get_count(); i++) {
            led_controller_set_color(i, r, g, b);
        }
        led_controller_update();
        
        // Avanzar hue según velocidad (0-100)
        hue = (hue + 1 + (config->speed / 20)) % 360;
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief Modo ATARDECER - Transición de temperatura blanca
 */
static void mode_sunset_task(void *pv)
{
    led_mode_config_t *config = (led_mode_config_t *)pv;
    uint16_t kelvin = 2700;
    int direction = 1;  // 1 = aumentando, -1 = disminuyendo
    
    ESP_LOGI(TAG, "🌅 Modo Atardecer iniciado");
    
    while (1) {
        // Actualizar temperatura
        kelvin += direction * (5 + config->speed / 10);
        
        // Cambiar dirección al llegar a los límites
        if (kelvin >= 6500) {
            kelvin = 6500;
            direction = -1;
        } else if (kelvin <= 2700) {
            kelvin = 2700;
            direction = 1;
        }
        
        // Aplicar temperatura (led_white ya actualiza los LEDs)
        led_white_set_temperature(kelvin);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Modo FIESTA - Colores aleatorios rápidos
 */
static void mode_party_task(void *pv)
{
    led_mode_config_t *config = (led_mode_config_t *)pv;
    uint8_t r, g, b;
    
    ESP_LOGI(TAG, "🎉 Modo Fiesta iniciado");
    
    while (1) {
        // Generar colores aleatorios
        r = rand() % 256;
        g = rand() % 256;
        b = rand() % 256;
        
        // Aplicar brillo configurado
        for (int i = 0; i < led_controller_get_count(); i++) {
            led_controller_set_color(i, r, g, b);
        }
        led_controller_update();
        
        // Velocidad: menor delay = más rápido
        vTaskDelay(pdMS_TO_TICKS(200 - config->speed));
    }
}

/**
 * @brief Modo RELAJACIÓN - Transiciones suaves
 */
static void mode_relax_task(void *pv)
{
    led_mode_config_t *config = (led_mode_config_t *)pv;
    uint16_t hue = 0;
    uint8_t r, g, b;
    uint8_t brightness = config->brightness;
    
    ESP_LOGI(TAG, "😌 Modo Relajación iniciado");
    
    while (1) {
        // Transición muy lenta de colores pastel
        led_controller_hsv2rgb(hue, 30, brightness, &r, &g, &b);  // Saturación baja = pastel
        
        for (int i = 0; i < led_controller_get_count(); i++) {
            led_controller_set_color(i, r, g, b);
        }
        led_controller_update();
        
        hue = (hue + 1) % 360;  // Muy lento
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Modo NOCTURNO - Luz cálida tenue
 */
static void mode_night_task(void *pv)
{
    led_mode_config_t *config = (led_mode_config_t *)pv;
    
    ESP_LOGI(TAG, "🌙 Modo Nocturno iniciado");
    
    // Usar la configuración recibida
    uint8_t brightness = config->brightness;
    if (brightness > 20) brightness = 20;  // Limitar a máximo 20% para nocturno
    
    led_controller_set_brightness(brightness);
    led_white_set_temperature(config->kelvin);  // Usar Kelvin configurado
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Modo TORMENTA - Efecto de relámpagos
 */
static void mode_storm_task(void *pv)
{
    led_mode_config_t *config = (led_mode_config_t *)pv;
    (void)config;  // Por ahora no usamos configuración en tormenta
    
    ESP_LOGI(TAG, "⛈️ Modo Tormenta iniciado");
    
    while (1) {
        // Fondo oscuro
        led_controller_clear();
        
        // Esperar tiempo aleatorio (1-5 segundos)
        vTaskDelay(pdMS_TO_TICKS(1000 + (rand() % 4000)));
        
        // Relámpago - usar brillo configurado
        uint8_t brightness = config ? config->brightness : 100;
        for (int i = 0; i < led_controller_get_count(); i++) {
            led_controller_set_color(i, brightness, brightness, brightness);
        }
        led_controller_update();
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Vuelta a oscuro
        led_controller_clear();
    }
}

// ==================== FUNCIONES PÚBLICAS ====================

esp_err_t led_modes_init(void)
{
    ESP_LOGI(TAG, "Inicializando módulo de modos/escenas");
    
    // Configuración por defecto
    current_config.speed = 50;
    current_config.brightness = 80;
    current_config.color_r = 255;
    current_config.color_g = 255;
    current_config.color_b = 255;
    current_config.kelvin = 4000;
    
    return ESP_OK;
}

esp_err_t led_modes_start(led_mode_t mode, const led_mode_config_t *config)
{
    // Validar modo
    if (mode >= LED_MODE_MAX) {
        ESP_LOGE(TAG, "Modo inválido: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Detener modo anterior si existe
    if (mode_task_handle != NULL) {
        led_modes_stop();
    }
    
    // Guardar configuración
    if (config != NULL) {
        memcpy(&current_config, config, sizeof(led_mode_config_t));
    }
    
    current_mode = mode;
    mode_active = true;
    
    // Crear tarea según el modo seleccionado
    TaskFunction_t task_func = NULL;
    const char *task_name = "mode_unknown";
    
    switch (mode) {
        case LED_MODE_RAINBOW:
            task_func = mode_rainbow_task;
            task_name = "mode_rainbow";
            break;
        case LED_MODE_SUNSET:
            task_func = mode_sunset_task;
            task_name = "mode_sunset";
            break;
        case LED_MODE_PARTY:
            task_func = mode_party_task;
            task_name = "mode_party";
            break;
        case LED_MODE_RELAX:
            task_func = mode_relax_task;
            task_name = "mode_relax";
            break;
        case LED_MODE_NIGHT:
            task_func = mode_night_task;
            task_name = "mode_night";
            break;
        case LED_MODE_STORM:
            task_func = mode_storm_task;
            task_name = "mode_storm";
            break;
        default:
            ESP_LOGW(TAG, "Modo %d no tiene tarea asociada", mode);
            mode_active = false;
            return ESP_OK;
    }
    
    // Crear tarea con la configuración como parámetro
    BaseType_t ret = xTaskCreate(
        task_func,
        task_name,
        4096,  // Stack size
        &current_config,
        1,     // Prioridad
        &mode_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Error creando tarea para modo %d", mode);
        mode_active = false;
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Modo %d iniciado correctamente", mode);
    return ESP_OK;
}

esp_err_t led_modes_stop(void)
{
    if (mode_task_handle != NULL) {
        vTaskDelete(mode_task_handle);
        mode_task_handle = NULL;
    }
    
    mode_active = false;
    current_mode = LED_MODE_SOLID;
    ESP_LOGI(TAG, "Modo detenido, volviendo a modo sólido");
    
    return ESP_OK;
}

led_mode_t led_modes_get_current(void)
{
    return current_mode;
}

esp_err_t led_modes_update_config(const led_mode_config_t *config)
{
    if (!mode_active) {
        ESP_LOGW(TAG, "No hay modo activo para actualizar");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (config != NULL) {
        memcpy(&current_config, config, sizeof(led_mode_config_t));
        ESP_LOGI(TAG, "Configuración actualizada: speed=%d, brightness=%d",
                 config->speed, config->brightness);
    }
    
    return ESP_OK;
}

bool led_modes_is_active(void)
{
    return mode_active;
}
