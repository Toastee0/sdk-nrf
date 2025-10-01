/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>

/**
 * @brief Configure WS2812 GPIO pin and strip parameters
 * @param pin GPIO pin number
 * @param port GPIO port number  
 * @param num_leds Number of LEDs in the strip
 */
void hrt_ws2812_configure(uint32_t pin, uint8_t port, uint32_t num_leds);

/**
 * @brief Send pixel data to WS2812 strip
 * @param pixel_data Pointer to GRB pixel data (3 bytes per pixel: G, R, B)
 *                   WS2812 protocol uses GRB color order, not RGB
 */
void hrt_ws2812_refresh(const uint8_t *pixel_data);

/**
 * @brief Send reset/latch signal to WS2812 strip
 *        Outputs low signal for >50μs to latch data
 */
void hrt_ws2812_clear(void);

#endif /* _HRT_H__ */
