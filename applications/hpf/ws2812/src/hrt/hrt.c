/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>

extern volatile uint16_t ws2812_pin_mask_arg;

/* WS2812 configuration stored globally */
static uint32_t ws2812_pin_mask = 0;
static uint32_t ws2812_num_leds = 0;

/* WS2812 protocol timing at 128MHz FLPR clock (7.8125ns per cycle) */
#define WS2812_T0H_CYCLES  45  /* ~350ns high for '0' bit */
#define WS2812_T0L_CYCLES  102 /* ~800ns low for '0' bit */
#define WS2812_T1H_CYCLES  90  /* ~700ns high for '1' bit */
#define WS2812_T1L_CYCLES  77  /* ~600ns low for '1' bit */
#define WS2812_RESET_CYCLES 6400 /* ~50us reset pulse */

/* Pixel data buffer - accessed via shared memory from ARM core */
static const uint8_t *pixel_data_ptr = NULL;

void hrt_ws2812_configure(uint32_t pin, uint8_t port, uint32_t num_leds)
{
	/* Calculate pin mask for VPR CSR operations */
	ws2812_pin_mask = (1U << pin);
	ws2812_num_leds = num_leds;
	
	/* Initialize pin to low state */
	uint16_t outs = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(outs & ~ws2812_pin_mask);
}

/* Precise delay using VPR timer */
static inline void delay_cycles(uint32_t cycles)
{
	uint16_t start = nrf_vpr_csr_vtim_simple_counter_get(0);
	uint16_t target = start + cycles;
	
	/* Handle counter wrap-around */
	if (target < start) {
		while (nrf_vpr_csr_vtim_simple_counter_get(0) >= start) {
			/* Wait for wrap */
		}
		while (nrf_vpr_csr_vtim_simple_counter_get(0) < target) {
			/* Wait for target */
		}
	} else {
		while (nrf_vpr_csr_vtim_simple_counter_get(0) < target) {
			/* Wait for target */
		}
	}
}

/* Send a '1' bit - 700ns high, 600ns low */
static inline void send_one_bit(void)
{
	uint16_t outs;
	
	/* Set pin high */
	outs = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(outs | ws2812_pin_mask);
	delay_cycles(WS2812_T1H_CYCLES);
	
	/* Set pin low */
	outs = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(outs & ~ws2812_pin_mask);
	delay_cycles(WS2812_T1L_CYCLES);
}

/* Send a '0' bit - 350ns high, 800ns low */
static inline void send_zero_bit(void)
{
	uint16_t outs;
	
	/* Set pin high */
	outs = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(outs | ws2812_pin_mask);
	delay_cycles(WS2812_T0H_CYCLES);
	
	/* Set pin low */
	outs = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(outs & ~ws2812_pin_mask);
	delay_cycles(WS2812_T0L_CYCLES);
}

void hrt_ws2812_refresh(const uint8_t *pixel_data)
{
	if (pixel_data == NULL) {
		return;
	}
	
	/* Store pixel data pointer for bit-banging */
	pixel_data_ptr = pixel_data;
	
	/* Send pixel data for configured number of LEDs */
	/* Each LED consumes 3 bytes (G, R, B in WS2812 format) */
	for (uint32_t led = 0; led < ws2812_num_leds; led++) {
		for (uint32_t byte = 0; byte < 3; byte++) {
			uint8_t data = pixel_data_ptr[led * 3 + byte];
			
			/* Send bits MSB first */
			for (int32_t bit = 7; bit >= 0; bit--) {
				if (data & (1U << bit)) {
					send_one_bit();
				} else {
					send_zero_bit();
				}
			}
		}
	}
}

void hrt_ws2812_clear(void)
{
	/* Send reset/latch pulse (>50μs low) */
	uint16_t outs = nrf_vpr_csr_vio_out_get();
	nrf_vpr_csr_vio_out_set(outs & ~ws2812_pin_mask);
	delay_cycles(WS2812_RESET_CYCLES);
}
