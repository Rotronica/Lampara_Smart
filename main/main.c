#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ws2812_driver.h"  // Driver de hardware
#include "led_controller.h" // Lógica de control
#include "led_color.h"
#include "led_white.h"
#include "led_modes.h"
#include "ble_foco.h"

static const char *TAG = "MAIN";

#define LED_GPIO 4
#define NUM_LEDS 1

// Callbacks BLE

// Callback para los colores
static void on_color_change(uint8_t r, uint8_t g, uint8_t b)
{
    ESP_LOGI(TAG, "🎨 Color recibido: RGB(%d,%d,%d)", r, g, b);
    led_color_set_all(r, g, b);
}

// Callback para el brillo
static void on_brightness_change(uint8_t brightness)
{
    ESP_LOGI(TAG, "💡 Brillo recibido: %d%%", brightness);
    led_controller_set_brightness(brightness);
    led_controller_update();
}

// Callback para registrar el color blanco
static void on_white_change(uint16_t kelvin)
{
    ESP_LOGI(TAG, "❄️🔥 Temperatura blanco: %dK", kelvin);
    led_white_set_temperature(kelvin);
}

// Callback para registrar los modos escogidos
static void on_mode_change(uint8_t mode, uint8_t speed)
{
    ESP_LOGI(TAG, "🎛️ Modo: %d, Velocidad: %d", mode, speed);

    // Detener modo anterior si lo hay
    if (led_modes_is_active())
    {
        led_modes_stop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Si es modo sólido (0), no hacer nada especial
    if (mode == 0)
    {
        ESP_LOGI(TAG, "Modo sólido activado");
        return;
    }

    // Configurar el modo seleccionado
    led_mode_config_t config = {
        .speed = speed,
        .brightness = led_controller_get_brightness(),
        .kelvin = 4000};

    // Obtener último color si está disponible
    uint8_t r, g, b;
    led_color_get_current(&r, &g, &b);
    config.color_r = r;
    config.color_g = g;
    config.color_b = b;

    // Validar que el modo esté en rango
    led_mode_t led_mode = mode;
    if (led_mode >= LED_MODE_MAX)
    {
        ESP_LOGE(TAG, "Modo inválido: %d", mode);
        return;
    }

    // Iniciar modo
    esp_err_t ret = led_modes_start(led_mode, &config);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Modo %d iniciado correctamente", led_mode);
    }
    else
    {
        ESP_LOGE(TAG, "Error iniciando modo %d", led_mode);
    }
}

// Callback para registrar si se conecto el cliente
static void on_connect(void)
{
    ESP_LOGI(TAG, "📱 Cliente conectado");
    // Efecto de bienvenida
    led_color_set_all(0, 255, 0); // Verde
    vTaskDelay(pdMS_TO_TICKS(500));
    led_color_set_all(0, 0, 0); // Apagar
}

// Callback para registrar que el cliente se desconecto
static void on_disconnect(void)
{
    ESP_LOGI(TAG, "📱 Cliente desconectado");
    if (led_modes_is_active())
    {
        led_modes_stop();
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== FOCO INTELIGENTE ===");

    // ==================== INICIALIZACIÓN ====================

    // 1. Inicializar DRIVER de hardware (WS2812)
    //    Este componente maneja el hardware RMT directamente
    ws2812_driver_config_t driver_config = {
        .gpio_pin = LED_GPIO,
        .num_leds = NUM_LEDS,
        .resolution_hz = 10000000 // 10MHz para WS2812
    };
    ESP_ERROR_CHECK(ws2812_driver_init(&driver_config));
    ESP_LOGI(TAG, "✅ Driver WS2812 inicializado");

    // 2. Inicializar CONTROLADOR de lógica
    //    Este componente maneja brillo y buffer de colores
    led_controller_config_t led_config = {
        .num_leds = NUM_LEDS};
    ESP_ERROR_CHECK(led_controller_init(&led_config));
    ESP_LOGI(TAG, "✅ Controlador LED inicializado");

    // 3. Inicializar módulos de alto nivel
    led_color_init();
    led_white_init();
    led_modes_init();
    ESP_LOGI(TAG, "✅ Módulos de alto nivel inicializados");

    // 4. Configurar TODOS los callbacks BLE
    ble_foco_callbacks_t cbs = {
        .on_color_change = on_color_change,
        .on_brightness_change = on_brightness_change,
        .on_mode_change = on_mode_change,
        .on_white_change = on_white_change,
        .on_connect = on_connect,
        .on_disconnect = on_disconnect};
    ble_foco_register_callbacks(&cbs);

    // 5. Inicializar BLE
    ESP_ERROR_CHECK(ble_foco_init());
    ESP_LOGI(TAG, "✅ BLE inicializado");

    ESP_LOGI(TAG, "✅ Sistema listo. Busca 'ESP_FOCO_TEST' en tu app BLE");

    // ==================== PRUEBA DE MODOS (opcional) ====================
    // Puedes comentar esta sección si no quieres la prueba automática
    ESP_LOGI(TAG, "Probando modo ARCOÍRIS durante 5 segundos...");
    led_mode_config_t test_config = {
        .speed = 50,
        .brightness = 80,
        .color_r = 255,
        .color_g = 0,
        .color_b = 0,
        .kelvin = 4000};
    led_modes_start(LED_MODE_RAINBOW, &test_config);
    vTaskDelay(pdMS_TO_TICKS(5000));
    led_modes_stop();
    led_color_set_all(0, 0, 0); // Apagar
    ESP_LOGI(TAG, "Prueba completada");

    // ==================== BUCLE PRINCIPAL ====================
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}