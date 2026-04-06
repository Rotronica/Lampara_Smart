#include "ws2812_driver.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/rmt_tx.h"
#include <string.h>
#include <stdlib.h>
#include <esp_heap_caps.h>
#include <portmacro.h>

static const char *TAG = "WS2812_DRV";

// ==================== ENCODER INTERNO ====================
// (antes led_strip_encoder.c)

typedef struct
{
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} ws2812_encoder_t;

static size_t rmt_encode_ws2812(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                const void *primary_data, size_t data_size,
                                rmt_encode_state_t *ret_state)
{
    ws2812_encoder_t *led_encoder = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_encoder->state)
    {
    case 0:
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            led_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    /* fall-through */ // ← AÑADE ESTA LÍNEA
    case 1:
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE)
        {
            led_encoder->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_ws2812_encoder(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *led_encoder = __containerof(encoder, ws2812_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_ws2812_encoder_reset(rmt_encoder_t *encoder)
{
    ws2812_encoder_t *led_encoder = __containerof(encoder, ws2812_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t ws2812_encoder_new(uint32_t resolution, rmt_encoder_handle_t *ret_encoder)
{
    ws2812_encoder_t *led_encoder = calloc(1, sizeof(ws2812_encoder_t));
    if (!led_encoder)
        return ESP_ERR_NO_MEM;

    led_encoder->base.encode = rmt_encode_ws2812;
    led_encoder->base.del = rmt_del_ws2812_encoder;
    led_encoder->base.reset = rmt_ws2812_encoder_reset;

    // Parámetros de temporización para WS2812
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 0.3 * resolution / 1000000, // T0H = 0.3us
            .level1 = 0,
            .duration1 = 0.9 * resolution / 1000000, // T0L = 0.9us
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 0.9 * resolution / 1000000, // T1H = 0.9us
            .level1 = 0,
            .duration1 = 0.3 * resolution / 1000000, // T1L = 0.3us
        },
        .flags.msb_first = 1};

    esp_err_t ret = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    if (ret != ESP_OK)
        goto err;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
    if (ret != ESP_OK)
        goto err;

    uint32_t reset_ticks = resolution / 1000000 * 50 / 2;
    led_encoder->reset_code = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &led_encoder->base;
    return ESP_OK;

err:
    if (led_encoder->bytes_encoder)
        rmt_del_encoder(led_encoder->bytes_encoder);
    if (led_encoder->copy_encoder)
        rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ret;
}

// ==================== DRIVER WS2812 ====================

typedef struct
{
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t rmt_encoder;
    uint8_t *buffer;
    int num_leds;
    bool initialized;
} ws2812_driver_t;

static ws2812_driver_t *s_drv = NULL;

// ==================== FUNCIONES PÚBLICAS ====================

esp_err_t ws2812_driver_init(const ws2812_driver_config_t *config)
{
    ESP_LOGI(TAG, "Inicializando driver WS2812");

    if (!config || config->num_leds <= 0)
        return ESP_ERR_INVALID_ARG;
    if (s_drv)
        ws2812_driver_deinit();

    s_drv = calloc(1, sizeof(ws2812_driver_t));
    if (!s_drv)
        return ESP_ERR_NO_MEM;

    s_drv->num_leds = config->num_leds;
    s_drv->buffer = heap_caps_malloc(config->num_leds * 3, MALLOC_CAP_DMA);
    if (!s_drv->buffer)
    {
        free(s_drv);
        s_drv = NULL;
        return ESP_ERR_NO_MEM;
    }
    memset(s_drv->buffer, 0, config->num_leds * 3);

    uint32_t resolution = config->resolution_hz ? config->resolution_hz : 10000000;

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = config->gpio_pin,
        .mem_block_symbols = 64,
        .resolution_hz = resolution,
        .trans_queue_depth = 4,
    };

    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &s_drv->rmt_channel);
    if (ret != ESP_OK)
        goto cleanup;

    ret = ws2812_encoder_new(resolution, &s_drv->rmt_encoder);
    if (ret != ESP_OK)
    {
        rmt_del_channel(s_drv->rmt_channel);
        goto cleanup;
    }

    ret = rmt_enable(s_drv->rmt_channel);
    if (ret != ESP_OK)
    {
        rmt_del_encoder(s_drv->rmt_encoder);
        rmt_del_channel(s_drv->rmt_channel);
        goto cleanup;
    }

    s_drv->initialized = true;
    ESP_LOGI(TAG, "Driver WS2812 listo: %d LEDs en GPIO %d", config->num_leds, config->gpio_pin);
    return ESP_OK;

cleanup:
    free(s_drv->buffer);
    free(s_drv);
    s_drv = NULL;
    return ret;
}

void ws2812_driver_set_color(int index, uint8_t g, uint8_t r, uint8_t b)
{
    if (!s_drv || !s_drv->initialized)
        return;
    if (index < 0 || index >= s_drv->num_leds)
        return;

    uint8_t *buf = s_drv->buffer + (index * 3);
    buf[0] = g;
    buf[1] = r;
    buf[2] = b;
}

void ws2812_driver_update(void)
{
    if (!s_drv || !s_drv->initialized)
        return;

    rmt_transmit_config_t tx_config = {.loop_count = 0};
    if (rmt_transmit(s_drv->rmt_channel, s_drv->rmt_encoder,
                     s_drv->buffer, s_drv->num_leds * 3, &tx_config) == ESP_OK)
    {
        rmt_tx_wait_all_done(s_drv->rmt_channel, portMAX_DELAY);
    }
}

void ws2812_driver_clear(void)
{
    if (!s_drv || !s_drv->initialized)
        return;
    memset(s_drv->buffer, 0, s_drv->num_leds * 3);
    ws2812_driver_update();
}

int ws2812_driver_get_count(void)
{
    return s_drv ? s_drv->num_leds : 0;
}

void ws2812_driver_deinit(void)
{
    if (!s_drv)
        return;
    if (s_drv->rmt_encoder)
        rmt_del_encoder(s_drv->rmt_encoder);
    if (s_drv->rmt_channel)
        rmt_del_channel(s_drv->rmt_channel);
    if (s_drv->buffer)
        free(s_drv->buffer);
    free(s_drv);
    s_drv = NULL;
}