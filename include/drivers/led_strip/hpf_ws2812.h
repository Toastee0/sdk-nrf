/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HPF_WS2812_H
#define HPF_WS2812_H

#include <drivers/nrfx_common.h>
#include <hpf/hpf_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief WS2812 HPF opcodes. */
typedef enum {
	HPF_WS2812_UPDATE = 0, /* Update LED strip with pixel data from shared memory. */
} hpf_ws2812_opcode_t;

/** @brief Maximum number of LEDs supported (based on SRAM allocation) */
#define HPF_WS2812_MAX_LEDS 1024

/** @brief WS2812 HPF control packet. */
typedef struct __packed {
	uint8_t opcode; /* WS2812 opcode - always HPF_WS2812_UPDATE. */
	uint32_t pin; /* GPIO pin number. */
	uint8_t port; /* GPIO port number. */
	uint32_t num_leds; /* Number of LEDs to update. */
} hpf_ws2812_control_packet_t;

/** @brief WS2812 pixel data - 3 bytes per LED (GRB format) */
typedef struct __packed {
	uint8_t g; /* Green component */
	uint8_t r; /* Red component */
	uint8_t b; /* Blue component */
} hpf_ws2812_pixel_t;

/** @brief WS2812 HPF shared data structure. */
typedef struct {
	struct hpf_shared_data_lock lock;
	hpf_ws2812_control_packet_t control;
	hpf_ws2812_pixel_t pixels[HPF_WS2812_MAX_LEDS];
} hpf_ws2812_mbox_data_t;

#ifdef __cplusplus
}
#endif

#endif /* HPF_WS2812_H */