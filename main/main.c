#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_controller.h"
#include "led_color.h"
#include "ble_foco.h"

static const char *TAG = "MAIN";

#define LED_GPIO    4
#define NUM_LEDS    1

// Callbacks BLE
static void on_color_change(uint8_t r, uint8_t g, uint8_t b)
{
    ESP_LOGI(TAG, "🎨 Color recibido: RGB(%d,%d,%d)", r, g, b);
    led_color_set_all(r, g, b);  // led_color ya usa led_controller internamente
}

static void on_brightness_change(uint8_t brightness)
{
    ESP_LOGI(TAG, "💡 Brillo recibido: %d%%", brightness);
    led_controller_set_brightness(brightness);  // ← Controla el brillo global
    led_controller_update();  // Refresca los LEDs con el nuevo brillo
}

static void on_connect(void)
{
    ESP_LOGI(TAG, "📱 Cliente conectado");
    // Opcional: hacer un efecto de bienvenida
    led_color_set_all(0, 255, 0);  // Verde
    vTaskDelay(pdMS_TO_TICKS(500));
    led_color_set_all(0, 0, 0);    // Apagar
}

static void on_disconnect(void)
{
    ESP_LOGI(TAG, "📱 Cliente desconectado");
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== FOCO INTELIGENTE ===");

    // 1. Inicializar controlador base de LEDs
    led_controller_config_t led_base_config = {
        .gpio_pin = LED_GPIO,
        .num_leds = NUM_LEDS,
        .resolution_hz = 10000000
    };
    ESP_ERROR_CHECK(led_controller_init(&led_base_config));

    // 2. Inicializar módulo de color
    led_color_init();

    // 3. Configurar TODOS los callbacks BLE
    ble_foco_callbacks_t cbs = {
        .on_color_change = on_color_change,
        .on_brightness_change = on_brightness_change,  // ← AHORA SÍ
        .on_mode_change = NULL,          // Por implementar
        .on_connect = on_connect,
        .on_disconnect = on_disconnect
    };
    ble_foco_register_callbacks(&cbs);

    // 4. Inicializar BLE
    ESP_ERROR_CHECK(ble_foco_init());

    ESP_LOGI(TAG, "✅ Sistema listo. Busca 'ESP_FOCO_TEST' en tu app BLE");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}