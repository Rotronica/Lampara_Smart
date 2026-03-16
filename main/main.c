#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_controller.h"
#include "ble_foco.h"

static const char *TAG = "MAIN";

#define LED_GPIO 4
#define NUM_LEDS 1

// Callback cuando llega color por BLE
static void on_color_from_ble(uint8_t r, uint8_t g, uint8_t b)
{
    ESP_LOGI(TAG, "Color recibido: RGB(%d,%d,%d)", r, g, b);
    
    // Aplicar a TODOS los LEDs (foco uniforme)
    for (int i = 0; i < NUM_LEDS; i++) {
        led_controller_set_color(i, r, g, b);
    }
    led_controller_update();
}

// Callback cuando llega brillo por BLE
static void on_brightness_from_ble(uint8_t brightness)
{
    ESP_LOGI(TAG, "Brillo recibido: %d%%", brightness);
    led_controller_set_brightness(brightness);
    led_controller_update();  // Re-aplica con nuevo brillo
}

void app_main(void)
{
    // 1. Inicializar LEDs
    led_controller_config_t led_config = {
        .gpio_pin = LED_GPIO,
        .num_leds = NUM_LEDS,
        .resolution_hz = 10000000  // 10MHz para WS2812
    };
    ESP_ERROR_CHECK(led_controller_init(&led_config));
    
    // 2. Secuencia de prueba (verde un momento)
    for (int i = 0; i < NUM_LEDS; i++) {
        led_controller_set_color(i, 0, 255, 0);
    }
    led_controller_update();
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_controller_clear();
    
    // 3. Configurar BLE
    ble_foco_callbacks_t cbs = {
        .on_color_change = on_color_from_ble,
        .on_brightness_change = on_brightness_from_ble,
        .on_connect = NULL,
        .on_disconnect = NULL,
        .on_mode_change = NULL
    };
    ble_foco_register_callbacks(&cbs);
    
    // 4. Inicializar BLE
    ESP_ERROR_CHECK(ble_foco_init());
    
    ESP_LOGI(TAG, "Sistema listo. Esperando conexiones BLE...");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}