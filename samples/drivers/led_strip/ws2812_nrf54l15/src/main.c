/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Simple WS2812 LED Strip Demo for nRF54L15
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ws2812_demo, LOG_LEVEL_INF);

#define LED_STRIP_NODE  DT_ALIAS(led_strip)
#define LED_STRIP_DEV   DEVICE_DT_GET(LED_STRIP_NODE)

#define STRIP_NUM_PIXELS    DT_PROP(LED_STRIP_NODE, chain_length)
#define DELAY_TIME          K_MSEC(100)

static struct led_rgb pixels[STRIP_NUM_PIXELS];

static void rainbow_effect(void)
{
    static uint8_t hue = 0;
    
    for (int i = 0; i < STRIP_NUM_PIXELS; i++) {
        uint8_t pixel_hue = hue + (i * 256 / STRIP_NUM_PIXELS);
        
        /* Simple HSV to RGB conversion */
        uint8_t sector = pixel_hue / 43;
        uint8_t remainder = (pixel_hue % 43) * 6;
        
        switch (sector) {
        case 0:
            pixels[i].r = 255;
            pixels[i].g = remainder;
            pixels[i].b = 0;
            break;
        case 1:
            pixels[i].r = 255 - remainder;
            pixels[i].g = 255;
            pixels[i].b = 0;
            break;
        case 2:
            pixels[i].r = 0;
            pixels[i].g = 255;
            pixels[i].b = remainder;
            break;
        case 3:
            pixels[i].r = 0;
            pixels[i].g = 255 - remainder;
            pixels[i].b = 255;
            break;
        case 4:
            pixels[i].r = remainder;
            pixels[i].g = 0;
            pixels[i].b = 255;
            break;
        case 5:
            pixels[i].r = 255;
            pixels[i].g = 0;
            pixels[i].b = 255 - remainder;
            break;
        }
        
        /* Reduce brightness */
        pixels[i].r /= 8;
        pixels[i].g /= 8;
        pixels[i].b /= 8;
    }
    
    hue += 4;
}

int main(void)
{
    int ret;

    if (!device_is_ready(LED_STRIP_DEV)) {
        LOG_ERR("LED strip device not ready");
        return -1;
    }

    LOG_INF("WS2812 demo starting with %d pixels", STRIP_NUM_PIXELS);

    while (1) {
        rainbow_effect();
        
        ret = led_strip_update_rgb(LED_STRIP_DEV, pixels, STRIP_NUM_PIXELS);
        if (ret < 0) {
            LOG_ERR("Failed to update LED strip: %d", ret);
        }
        
        k_sleep(DELAY_TIME);
    }

    return 0;
}