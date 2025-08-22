/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include "hrt.h"
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>
#include <zephyr/sys/byteorder.h>

/* Hardware requirement, to get n shifts SHIFTCNTB register has to be set to n-1*/
#define SHIFTCNTB_VALUE(shift_count) (shift_count - 1)

/* WS2812 specific constants */
#define WS2812_DATA_PIN_NUM    1  /* D0_VIO pin used for WS2812 data */
#define D0_VIO		    1  /* WS2812 data output pin */
#define WS2812_DATA_VIO     D0_VIO  /* Primary WS2812 data pin */

#define IN_REG_EMPTY UINT16_MAX


static const nrf_vpr_csr_vio_shift_ctrl_t write_final_shift_ctrl_cfg = {
	.shift_count = 1,
	.out_mode = NRF_VPR_CSR_VIO_SHIFT_NONE,
	.frame_width = 1,
	.in_mode = NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS,
};

static nrf_vpr_csr_vio_shift_ctrl_t xfer_shift_ctrl = {
	.shift_count = SHIFTCNTB_VALUE(BITS_IN_WORD),
	.out_mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
	.frame_width = 1,
	.in_mode = NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS,
};

NRF_STATIC_INLINE bool is_counter_running(uint8_t counter)
{
	/* This may not work for higher frequencies but
	 * there is no other way to check if timer is running.
	 * Tested it down to counter value 2 which is 21.333333MHz.
	 */
	uint16_t cnt = nrf_vpr_csr_vtim_simple_counter_get(counter);

	return cnt != nrf_vpr_csr_vtim_simple_counter_get(counter);
}

NRF_STATIC_INLINE uint32_t get_next_shift_count(volatile hrt_xfer_t *hrt_xfer_params,
						uint32_t word_count, uint32_t index)
{
	/* WS2812 always uses full 32-bit words - no partial LED data */
	(void)hrt_xfer_params; /* Suppress unused parameter warning */
	(void)word_count;      /* Suppress unused parameter warning */
	(void)index;           /* Suppress unused parameter warning */
	
	return SHIFTCNTB_VALUE(BITS_IN_WORD);
}

/* WS2812-specific helper function to calculate VPR counter value from frequency */
NRF_STATIC_INLINE uint16_t ws2812_calculate_counter_value(uint32_t vpr_freq_hz, uint32_t bit_freq_hz)
{
	/* Calculate counter value to achieve desired bit frequency 
	 * VPR counter value = (VPR_frequency / desired_bit_frequency) - 1
	 * For WS2812: bit frequency ≈ 800kHz (1.25μs per bit)
	 */
	uint32_t counter_val = (vpr_freq_hz / bit_freq_hz);
	return (counter_val > 0) ? (counter_val - 1) : 0;
}

static void hrt_tx(volatile hrt_xfer_data_t *xfer_data, bool *counter_running,
		   uint16_t counter_value)
{
	if (xfer_data->word_count == 0) {
		return;
	}

	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,		
	};
	
	uint32_t data;

	/* WS2812 is always single-wire, so BITS_IN_WORD for full 32-bit words */
	xfer_shift_ctrl.shift_count = SHIFTCNTB_VALUE(BITS_IN_WORD);

	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&xfer_shift_ctrl);

	for (uint32_t i = 0; i < xfer_data->word_count; i++) {

		/* WS2812 always has complete LED data - no partial words */
		if (xfer_data->data == NULL) {
			data = 0;
		} else {
			data = ((uint32_t *)xfer_data->data)[i];
		}

		switch (xfer_data->fun_out) {
		case HRT_FUN_OUT_WORD:
			nrf_vpr_csr_vio_out_buffered_reversed_word_set(data);
			break;
		case HRT_FUN_OUT_BYTE:
			nrf_vpr_csr_vio_out_buffered_reversed_byte_set(data);
			break;
		default:
			break;
		}

		if ((i == 0) && (!*counter_running)) {
			/*
			 * Temporary fix for max frequency. Top value can be set to 0,
			 * but initial value cannot, because counter will not start.
			 */
			if (counter_value == 0) {
				counter_value = 1;
			}
			/* Start counter */
			nrf_vpr_csr_vtim_simple_counter_set(0, counter_value);
			*counter_running = true;
		}
	}
}

void hrt_write(volatile hrt_xfer_t *hrt_xfer_params)
{
	bool counter_running = false;
	nrf_vpr_csr_vio_mode_out_t out_mode = {
		.mode = NRF_VPR_CSR_VIO_SHIFT_OUTB_TOGGLE,
	};

	/* Configure GPIO direction for WS2812 output pin */
	nrf_vpr_csr_vio_dir_set(hrt_xfer_params->tx_direction_mask);

	/* WS2812 is always single-wire protocol */
	xfer_shift_ctrl.frame_width = 1;

	/* Configure VPR timer for WS2812 bit timing */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);
	nrf_vpr_csr_vtim_simple_counter_top_set(0, hrt_xfer_params->counter_value);
	nrf_vpr_csr_vio_mode_in_set(NRF_VPR_CSR_VIO_MODE_IN_CONTINUOUS);

	nrf_vpr_csr_vio_mode_out_set(&out_mode);

	/* Set shift count for WS2812 - always full 32-bit words */
	nrf_vpr_csr_vio_shift_cnt_out_set(BITS_IN_WORD);

	/* WS2812 data line starts low - ensure clean initial state */
	nrf_vpr_csr_vio_out_clear_set(BIT(WS2812_DATA_VIO));

	/* Transfer WS2812 data - only data phase needed */
	hrt_tx(&hrt_xfer_params->xfer_data[HRT_FE_DATA],
	       &counter_running, hrt_xfer_params->counter_value);

	/* WS2812 protocol completion - ensure clean termination */
	nrf_vpr_csr_vio_shift_ctrl_buffered_set(&write_final_shift_ctrl_cfg);
	
	/* Wait for final shift to complete */
	while (nrf_vpr_csr_vio_shift_cnt_out_get() != 0) {
	}
	
	/* Stop the timer */
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_STOP);

	/* Reset counter for next WS2812 transmission */
	nrf_vpr_csr_vtim_simple_counter_set(0, 0);

	/* Ensure WS2812 data line is low for reset period */
	nrf_vpr_csr_vio_out_clear_set(BIT(WS2812_DATA_VIO));
	
	/* WS2812 reset period (50μs low) will be handled by application timing */
}


