#include "led_color.h"
#include "led_controller.h"
#include "esp_log.h"

static const char *TAG = "LED_COLOR";

static uint8_t current_r = 255;
static uint8_t current_g = 255;
static uint8_t current_b = 255;

esp_err_t led_color_init(void)
{
    ESP_LOGI(TAG, "Módulo de color iniciado");
    return ESP_OK;  // led_controller ya fue inicializado en main
}

void led_color_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    current_r = r;
    current_g = g;
    current_b = b;

    int num_leds = led_controller_get_count();
    for (int i = 0; i < num_leds; i++) {
        led_controller_set_color(i, r, g, b);
    }
    led_controller_update();
}

void led_color_set_led(int index, uint8_t r, uint8_t g, uint8_t b)
{
    led_controller_set_color(index, r, g, b);
    // No se actualiza automáticamente para permitir múltiples cambios
}

void led_color_get_current(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r) *r = current_r;
    if (g) *g = current_g;
    if (b) *b = current_b;
}

void led_color_rainbow_step(void)
{
    static uint16_t hue = 0;
    uint8_t r, g, b;
    led_controller_hsv2rgb(hue, 100, 100, &r, &g, &b);
    led_color_set_all(r, g, b);
    hue = (hue + 5) % 360;
}

void led_color_update(void)
{
    led_controller_update();
}