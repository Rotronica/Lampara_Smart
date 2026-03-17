#include "led_white.h"
#include "led_controller.h"
#include "esp_log.h"
#include "ble_foco.h"  // Para poder llamar a ble_foco_update_white()
#include <math.h>

static const char *TAG = "LED_WHITE";

static uint16_t current_kelvin = 4000;  // Valor por defecto (neutro)

esp_err_t led_white_init(void)
{
    ESP_LOGI(TAG, "Módulo de blanco iniciado");
    return ESP_OK;
}

void led_white_kelvin_to_rgb(uint16_t kelvin, uint8_t *r, uint8_t *g, uint8_t *b)
{
    // Algoritmo de conversión de temperatura de color a RGB
    // Basado en la aproximación de Tanner Helland
    float temp = kelvin / 100.0f;
    float red, green, blue;

    // Rojo
    if (temp <= 66) {
        red = 255;
    } else {
        red = temp - 60;
        red = 329.698727446f * powf(red, -0.1332047592f);
        if (red < 0) red = 0;
        if (red > 255) red = 255;
    }

    // Verde
    if (temp <= 66) {
        green = temp;
        green = 99.4708025861f * logf(green) - 161.1195681661f;
    } else {
        green = temp - 60;
        green = 288.1221695283f * powf(green, -0.0755148492f);
    }
    if (green < 0) green = 0;
    if (green > 255) green = 255;

    // Azul
    if (temp >= 66) {
        blue = 255;
    } else if (temp <= 19) {
        blue = 0;
    } else {
        blue = temp - 10;
        blue = 138.5177312231f * logf(blue) - 305.0447927307f;
        if (blue < 0) blue = 0;
        if (blue > 255) blue = 255;
    }

    if (r) *r = (uint8_t)red;
    if (g) *g = (uint8_t)green;
    if (b) *b = (uint8_t)blue;
}

void led_white_set_temperature(uint16_t kelvin)
{
    // Limitar al rango válido
    if (kelvin < WHITE_MIN_KELVIN) kelvin = WHITE_MIN_KELVIN;
    if (kelvin > WHITE_MAX_KELVIN) kelvin = WHITE_MAX_KELVIN;

    current_kelvin = kelvin;

    // Convertir Kelvin a RGB
    uint8_t r, g, b;
    led_white_kelvin_to_rgb(kelvin, &r, &g, &b);

    // Aplicar a todos los LEDs
    int num_leds = led_controller_get_count();
    for (int i = 0; i < num_leds; i++) {
        led_controller_set_color(i, r, g, b);
    }
    led_controller_update();

    ESP_LOGI(TAG, "Temperatura establecida: %dK → RGB(%d,%d,%d)", kelvin, r, g, b);
     // Sincronizar con BLE (si el módulo está disponible)
    ble_foco_update_white(kelvin);  // ← ACTUALIZA EL VALOR EN GATT
}

uint16_t led_white_get_temperature(void)
{
    return current_kelvin;
}