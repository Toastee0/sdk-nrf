/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DRIVERS_WS2812_HPF_WS2812_H
#define DRIVERS_WS2812_HPF_WS2812_H

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/ws2812.h>
#include <hal/nrf_timer.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SOC_NRF54L15
#define HPF_WS2812_PINS_MAX	 1
#else
#error "Unsupported SoC for HPF WS2812"
#endif

/** @brief WS2812 opcodes. */
typedef enum {
	HPF_WS2812_EP_BOUNDED = 0,
	HPF_WS2812_CONFIG_TIMER_PTR, /* hpf_ws2812_flpr_timer_msg_t */
	HPF_WS2812_CONFIG_PINS,      /* hpf_ws2812_pinctrl_soc_pin_msg_t */
	HPF_WS2812_CONFIG_TIMING,    /* hpf_ws2812_timing_config_msg_t */
	HPF_WS2812_TX,	            /* hpf_ws2812_xfer_packet_msg_t + data buffer at the end */
	HPF_WS2812_HPF_APP_HARD_FAULT,
	HPF_WS2812_WRONG_OPCODE,
	HPF_WS2812_OPCODES_COUNT = HPF_WS2812_WRONG_OPCODE,
	HPF_WS2812_OPCODES_MAX = 0xFFFFFFFFU,
} hpf_ws2812_opcode_t;

typedef struct {
	uint16_t t0h_ns;        /* T0H timing in nanoseconds (typical 350ns) */
	uint16_t t0l_ns;        /* T0L timing in nanoseconds (typical 800ns) */
	uint16_t t1h_ns;        /* T1H timing in nanoseconds (typical 700ns) */
	uint16_t t1l_ns;        /* T1L timing in nanoseconds (typical 600ns) */
	uint32_t reset_ns;      /* Reset timing in nanoseconds (typical 50000ns) */
	enum WS2812_mode color_order; /* GRB, RGB, RGBW */
} hpf_ws2812_timing_config_t;

typedef struct {
	uint8_t device_index;
	uint8_t command_length;
	uint8_t address_length;
	bool hold_ce;
	uint16_t tx_dummy;
	uint16_t rx_dummy;
} hpf_ws2812_xfer_config_t;

typedef struct {
	hpf_ws2812_opcode_t opcode; /* HPF_WS2812_CONFIG_TIMER_PTR */
	NRF_TIMER_Type *timer_ptr;
} hpf_ws2812_flpr_timer_msg_t;

typedef struct {
	hpf_ws2812_opcode_t opcode; /* HPF_WS2812_CONFIG_PINS */
	uint8_t pins_count;
	pinctrl_soc_pin_t pin[HPF_WS2812_PINS_MAX];
} hpf_ws2812_pinctrl_soc_pin_msg_t;

typedef struct {
	hpf_ws2812_opcode_t opcode; /* HPF_WS2812_CONFIG_TIMING */
	hpf_ws2812_timing_config_t timing_config;
} hpf_ws2812_timing_config_msg_t;

typedef struct {
	hpf_ws2812_opcode_t opcode; /* HPF_WS2812_TX */
	uint32_t command;
	uint32_t address;
	uint32_t num_bytes; /* Size of data */
#if (defined(CONFIG_WS2812_HPF_IPC_NO_COPY) || defined(CONFIG_HPF_WS2812_IPC_NO_COPY))
	uint8_t *data;
#else
	uint8_t data[]; /* Variable length data field at the end of packet. */
#endif
} hpf_ws2812_xfer_packet_msg_t;

typedef struct {
	hpf_ws2812_opcode_t opcode;
	uint8_t data;
} hpf_ws2812_flpr_response_msg_t;

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_WS2812_HPF_WS2812_H */
