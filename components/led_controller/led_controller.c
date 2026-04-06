#include "led_controller.h"
#include "ws2812_driver.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "LED_CTRL";

// Estructura interna del controlador (solo lógica)
typedef struct
{
    uint8_t *current_colors; // Colores originales (orden RGB)
    int num_leds;
    uint8_t brightness;
    bool initialized;
} led_controller_t;

static led_controller_t *s_led = NULL;

// ==================== FUNCIONES PRIVADAS ====================

static void apply_brightness(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s_led && s_led->brightness < 100)
    {
        *r = (*r * s_led->brightness) / 100;
        *g = (*g * s_led->brightness) / 100;
        *b = (*b * s_led->brightness) / 100;
    }
}

static void rebuild_driver_buffer(void)
{
    if (!s_led || !s_led->initialized)
        return;

    for (int i = 0; i < s_led->num_leds; i++)
    {
        uint8_t *orig = s_led->current_colors + (i * 3);
        uint8_t r = orig[0], g = orig[1], b = orig[2];
        apply_brightness(&r, &g, &b);
        // El driver espera orden GRB
        ws2812_driver_set_color(i, g, r, b);
    }
}

// ==================== FUNCIONES PÚBLICAS ====================

esp_err_t led_controller_init(const led_controller_config_t *config)
{
    ESP_LOGI(TAG, "Inicializando controlador de LEDs (lógica)");

    if (!config || config->num_leds <= 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_led)
    {
        if (s_led->current_colors)
            free(s_led->current_colors);
        free(s_led);
    }

    s_led = calloc(1, sizeof(led_controller_t));
    if (!s_led)
        return ESP_ERR_NO_MEM;

    s_led->num_leds = config->num_leds;
    s_led->brightness = 100;

    s_led->current_colors = malloc(config->num_leds * 3);
    if (!s_led->current_colors)
    {
        ESP_LOGE(TAG, "Error reservando memoria");
        free(s_led);
        s_led = NULL;
        return ESP_ERR_NO_MEM;
    }

    memset(s_led->current_colors, 0, config->num_leds * 3);
    s_led->initialized = true;

    ESP_LOGI(TAG, "Controlador listo para %d LEDs", config->num_leds);
    return ESP_OK;
}

esp_err_t led_controller_set_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led || !s_led->initialized)
        return ESP_ERR_INVALID_STATE;
    if (index < 0 || index >= s_led->num_leds)
        return ESP_ERR_INVALID_ARG;

    uint8_t *orig = s_led->current_colors + (index * 3);
    orig[0] = r;
    orig[1] = g;
    orig[2] = b;

    // Reconstruir buffer del driver con el nuevo brillo
    uint8_t ra = r, ga = g, ba = b;
    apply_brightness(&ra, &ga, &ba);
    ws2812_driver_set_color(index, ga, ra, ba);

    return ESP_OK;
}

esp_err_t led_controller_update(void)
{
    if (!s_led || !s_led->initialized)
        return ESP_ERR_INVALID_STATE;
    ws2812_driver_update();
    return ESP_OK;
}

esp_err_t led_controller_set_brightness(uint8_t brightness)
{
    if (!s_led || !s_led->initialized)
        return ESP_ERR_INVALID_STATE;
    if (brightness > 100)
        brightness = 100;

    if (s_led->brightness != brightness)
    {
        s_led->brightness = brightness;
        rebuild_driver_buffer();
    }
    return ESP_OK;
}

uint8_t led_controller_get_brightness(void)
{
    return s_led ? s_led->brightness : 0;
}

esp_err_t led_controller_clear(void)
{
    if (!s_led || !s_led->initialized)
        return ESP_ERR_INVALID_STATE;
    memset(s_led->current_colors, 0, s_led->num_leds * 3);
    ws2812_driver_clear();
    return ESP_OK;
}

esp_err_t led_controller_get_color(int index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (!s_led || !s_led->initialized)
        return ESP_ERR_INVALID_STATE;
    if (index < 0 || index >= s_led->num_leds)
        return ESP_ERR_INVALID_ARG;

    uint8_t *orig = s_led->current_colors + (index * 3);
    if (r)
        *r = orig[0];
    if (g)
        *g = orig[1];
    if (b)
        *b = orig[2];
    return ESP_OK;
}

int led_controller_get_count(void)
{
    return s_led ? s_led->num_leds : 0;
}

void led_controller_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h %= 360;
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;
    uint32_t i = h / 60;
    uint32_t diff = h % 60;
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    uint32_t rv, gv, bv;
    switch (i)
    {
    case 0:
        rv = rgb_max;
        gv = rgb_min + rgb_adj;
        bv = rgb_min;
        break;
    case 1:
        rv = rgb_max - rgb_adj;
        gv = rgb_max;
        bv = rgb_min;
        break;
    case 2:
        rv = rgb_min;
        gv = rgb_max;
        bv = rgb_min + rgb_adj;
        break;
    case 3:
        rv = rgb_min;
        gv = rgb_max - rgb_adj;
        bv = rgb_max;
        break;
    case 4:
        rv = rgb_min + rgb_adj;
        gv = rgb_min;
        bv = rgb_max;
        break;
    default:
        rv = rgb_max;
        gv = rgb_min;
        bv = rgb_max - rgb_adj;
        break;
    }

    if (r)
        *r = rv;
    if (g)
        *g = gv;
    if (b)
        *b = bv;
}