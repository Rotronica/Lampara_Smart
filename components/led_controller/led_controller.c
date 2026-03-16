#include "led_controller.h"
#include "led_strip_encoder.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/rmt_tx.h"
#include <string.h>
#include <stdlib.h>
#include <portmacro.h>

static const char *TAG = "LED_CTRL";

// Estructura interna del controlador (oculta en el .c)
typedef struct {
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t rmt_encoder;
    uint8_t *pixel_buffer;
    int num_leds;
    uint8_t brightness;
    uint8_t *current_colors;  // Almacena colores sin brillo aplicado
    bool initialized;
} led_controller_t;

static led_controller_t *s_led = NULL;

// ==================== FUNCIONES PRIVADAS ====================

/**
 * @brief Aplica el brillo actual a un color RGB
 */
static void apply_brightness(uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s_led && s_led->brightness < 100) {
        *r = (*r * s_led->brightness) / 100;
        *g = (*g * s_led->brightness) / 100;
        *b = (*b * s_led->brightness) / 100;
    }
}

/**
 * @brief Convierte un color RGB al orden GRB que espera WS2812 y lo copia al buffer
 */
static void rgb_to_grb_buffer(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led || !s_led->pixel_buffer) return;
    
    uint8_t *buf = s_led->pixel_buffer + (index * 3);
    buf[0] = g;  // WS2812 espera Green primero
    buf[1] = r;  // Luego Red
    buf[2] = b;  // Finalmente Blue
}

// ==================== FUNCIONES PÚBLICAS ====================

esp_err_t led_controller_init(const led_controller_config_t *config)
{
    ESP_LOGI(TAG, "Inicializando controlador de LEDs");
    
    if (!config || config->num_leds <= 0) {
        ESP_LOGE(TAG, "Configuración inválida");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Liberar instancia anterior si existe
    if (s_led) {
        led_controller_clear();
        if (s_led->pixel_buffer) free(s_led->pixel_buffer);
        if (s_led->current_colors) free(s_led->current_colors);
        free(s_led);
    }
    
    // Crear nueva instancia
    s_led = (led_controller_t*)calloc(1, sizeof(led_controller_t));
    if (!s_led) {
        ESP_LOGE(TAG, "No memory for controller");
        return ESP_ERR_NO_MEM;
    }
    
    s_led->num_leds = config->num_leds;
    s_led->brightness = 100;  // Brillo por defecto al máximo
    
    // Asignar memoria para buffers (usar DMA-capable memory para mejor rendimiento)
    s_led->pixel_buffer = heap_caps_malloc(config->num_leds * 3, MALLOC_CAP_DMA);
    s_led->current_colors = (uint8_t*)malloc(config->num_leds * 3);
    
    if (!s_led->pixel_buffer || !s_led->current_colors) {
        ESP_LOGE(TAG, "No memory for pixel buffers");
        if (s_led->pixel_buffer) free(s_led->pixel_buffer);
        if (s_led->current_colors) free(s_led->current_colors);
        free(s_led);
        s_led = NULL;
        return ESP_ERR_NO_MEM;
    }
    
    memset(s_led->pixel_buffer, 0, config->num_leds * 3);
    memset(s_led->current_colors, 0, config->num_leds * 3);
    
    // Configurar canal RMT
    uint32_t resolution = config->resolution_hz;
    if (resolution == 0) resolution = 10000000;  // 10MHz por defecto
    
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = config->gpio_pin,
        .mem_block_symbols = 64,
        .resolution_hz = resolution,
        .trans_queue_depth = 4,
    };
    
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &s_led->rmt_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creando canal RMT: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    
    // Configurar encoder para WS2812
    led_strip_encoder_config_t encoder_config = {
        .resolution = resolution,
    };
    
    ret = rmt_new_led_strip_encoder(&encoder_config, &s_led->rmt_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error creando encoder LED: %s", esp_err_to_name(ret));
        rmt_del_channel(s_led->rmt_channel);
        goto cleanup;
    }
    
    // Habilitar canal
    ret = rmt_enable(s_led->rmt_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error habilitando canal RMT: %s", esp_err_to_name(ret));
        rmt_del_encoder(s_led->rmt_encoder);
        rmt_del_channel(s_led->rmt_channel);
        goto cleanup;
    }
    
    s_led->initialized = true;
    ESP_LOGI(TAG, "Controlador inicializado para %d LEDs en GPIO %d", 
             config->num_leds, config->gpio_pin);
    return ESP_OK;

cleanup:
    free(s_led->pixel_buffer);
    free(s_led->current_colors);
    free(s_led);
    s_led = NULL;
    return ret;
}

esp_err_t led_controller_set_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led || !s_led->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (index < 0 || index >= s_led->num_leds) {
        ESP_LOGE(TAG, "Índice %d fuera de rango (0-%d)", index, s_led->num_leds - 1);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Guardar color original (sin brillo)
    uint8_t *orig = s_led->current_colors + (index * 3);
    orig[0] = r;
    orig[1] = g;
    orig[2] = b;
    
    // Aplicar brillo y convertir a GRB en el buffer de envío
    uint8_t ra = r, ga = g, ba = b;
    apply_brightness(&ra, &ga, &ba);
    rgb_to_grb_buffer(index, ra, ga, ba);
    
    return ESP_OK;
}

esp_err_t led_controller_update(void)
{
    if (!s_led || !s_led->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    
    esp_err_t ret = rmt_transmit(s_led->rmt_channel, 
                                  s_led->rmt_encoder,
                                  s_led->pixel_buffer,
                                  s_led->num_leds * 3,
                                  &tx_config);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error en transmisión RMT: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = rmt_tx_wait_all_done(s_led->rmt_channel, portMAX_DELAY);
    return ret;
}

esp_err_t led_controller_set_brightness(uint8_t brightness)
{
    if (!s_led || !s_led->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (brightness > 100) brightness = 100;
    
    if (s_led->brightness != brightness) {
        s_led->brightness = brightness;
        
        // Reconstruir buffer con nuevo brillo
        for (int i = 0; i < s_led->num_leds; i++) {
            uint8_t *orig = s_led->current_colors + (i * 3);
            uint8_t r = orig[0], g = orig[1], b = orig[2];
            uint8_t ra = r, ga = g, ba = b;
            apply_brightness(&ra, &ga, &ba);
            rgb_to_grb_buffer(i, ra, ga, ba);
        }
        
        ESP_LOGD(TAG, "Brillo actualizado a %d%%", brightness);
    }
    
    return ESP_OK;
}

esp_err_t led_controller_clear(void)
{
    if (!s_led || !s_led->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    memset(s_led->current_colors, 0, s_led->num_leds * 3);
    memset(s_led->pixel_buffer, 0, s_led->num_leds * 3);
    
    return led_controller_update();
}

esp_err_t led_controller_get_color(int index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (!s_led || !s_led->initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (index < 0 || index >= s_led->num_leds) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t *orig = s_led->current_colors + (index * 3);
    if (r) *r = orig[0];
    if (g) *g = orig[1];
    if (b) *b = orig[2];
    
    return ESP_OK;
}

void led_controller_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h %= 360;
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;
    uint32_t i = h / 60;
    uint32_t diff = h % 60;
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    uint32_t r_val, g_val, b_val;
    
    switch (i) {
        case 0:
            r_val = rgb_max;
            g_val = rgb_min + rgb_adj;
            b_val = rgb_min;
            break;
        case 1:
            r_val = rgb_max - rgb_adj;
            g_val = rgb_max;
            b_val = rgb_min;
            break;
        case 2:
            r_val = rgb_min;
            g_val = rgb_max;
            b_val = rgb_min + rgb_adj;
            break;
        case 3:
            r_val = rgb_min;
            g_val = rgb_max - rgb_adj;
            b_val = rgb_max;
            break;
        case 4:
            r_val = rgb_min + rgb_adj;
            g_val = rgb_min;
            b_val = rgb_max;
            break;
        default:
            r_val = rgb_max;
            g_val = rgb_min;
            b_val = rgb_max - rgb_adj;
            break;
    }
    
    if (r) *r = r_val;
    if (g) *g = g_val;
    if (b) *b = b_val;
}
