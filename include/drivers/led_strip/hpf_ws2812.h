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
	HPF_WS2812_PIN_CONFIGURE = 0, /* Configure GPIO pin and number of LEDs for this instance. */
	HPF_WS2812_REFRESH = 1, /* Update LED strip with pixel data from the message data. */
	HPF_WS2812_CLEAR = 2, /* Set GPIO low for reset period. */
} hpf_ws2812_opcode_t;

/** @brief WS2812 HPF data packet. */
typedef struct __packed {
	uint8_t opcode; /* WS2812 opcode. */
	uint32_t pin; /* Pin number when opcode is HPF_WS2812_PIN_CONFIGURE. */
	uint8_t port; /* Port number. */
	uint32_t numleds; /* Number of LEDs when opcode is HPF_WS2812_PIN_CONFIGURE.
			 * Not used in other cases.
			 */
} hpf_ws2812_data_packet_t;

typedef struct {
	struct hpf_shared_data_lock lock;
	hpf_ws2812_data_packet_t data;
} hpf_ws2812_mbox_data_t;

#ifdef __cplusplus
}
#endif

#endif /* HPF_WS2812_H */