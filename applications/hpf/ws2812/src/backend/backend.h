/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BACKEND_H__
#define _BACKEND_H__

#include <drivers/led_strip/hpf_ws2812.h>

#if !defined(CONFIG_HPF_WS2812_BACKEND_ICMSG) && \
	!defined(CONFIG_HPF_WS2812_BACKEND_MBOX) && \
	!defined(CONFIG_HPF_WS2812_BACKEND_ICBMSG)
#error "Define communication backend type"
#endif

/**
 * @brief Callback function called by backend when new packet arrives.
 *
 * @param control Control packet with opcode, pin, port, and LED count.
 * @param pixels Array of pixel data (GRB format).
 */
typedef void (*backend_callback_t)(hpf_ws2812_control_packet_t *control, 
                                   hpf_ws2812_pixel_t *pixels);

/**
 * @brief Initialize backend.
 *
 * @param callback Function to be called when new packet arrives.
 */
int backend_init(backend_callback_t callback);


#endif /* BACKEND_H__ */
