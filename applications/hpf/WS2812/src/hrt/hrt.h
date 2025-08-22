/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _HRT_H__
#define _HRT_H__

#include <stdint.h>
#include <stdbool.h>
#include <drivers/ws2812/hpf_ws2812.h>
#include <zephyr/drivers/ws2812.h>

#define VPRCSR_NORDIC_OUT_HIGH 1
#define VPRCSR_NORDIC_OUT_LOW  0

#define VPRCSR_NORDIC_DIR_OUTPUT 1
#define VPRCSR_NORDIC_DIR_INPUT	 0

#define BITS_IN_WORD 32
#define BITS_IN_BYTE 8

/* WS2812 specific timing constants */
#define WS2812_T0H_NS    350  /* T0H: 350ns ± 150ns */
#define WS2812_T0L_NS    800  /* T0L: 800ns ± 150ns */
#define WS2812_T1H_NS    700  /* T1H: 700ns ± 150ns */
#define WS2812_T1L_NS    600  /* T1L: 600ns ± 150ns */
#define WS2812_RESET_US  50   /* Reset: >50μs */

typedef enum {
	HRT_FE_COMMAND,    /* Legacy - not used for WS2812 */
	HRT_FE_ADDRESS,    /* Legacy - not used for WS2812 */
	HRT_FE_DUMMY_CYCLES, /* Legacy - not used for WS2812 */
	HRT_FE_DATA,       /* WS2812 uses only data phase */
	HRT_FE_MAX
} hrt_frame_element_t;

typedef enum {
	HRT_FUN_OUT_BYTE,
	HRT_FUN_OUT_WORD,
} hrt_fun_out_t;

/** @brief Structure for holding bus width of different xfer parts 
 *  For WS2812, only data width is used (single wire protocol)
 */
typedef struct {
	uint8_t command;      /* Legacy - not used for WS2812 */
	uint8_t address;      /* Legacy - not used for WS2812 */
	uint8_t dummy_cycles; /* Legacy - not used for WS2812 */
	uint8_t data;         /* WS2812 data width (always 1 for single wire) */
} hrt_xfer_bus_widths_t;

typedef struct {
	/** @brief Buffer for RX/TX data */
	volatile uint8_t *data;

	/** @brief Data length in 4 byte words,
	 *         calculated as CEIL(buffer_length_bits/32).
	 */
	uint32_t word_count;

	/** @brief Amount of clock pulses for last word.
	 *         Due to hardware limitation, in case when last word clock pulse count is 1,
	 *         the penultimate word has to share its bits with last word,
	 *         for example:
	 *         buffer length = 36bits,
	 *         bus_width = QUAD,
	 *         last_word_clocks would be:(buffer_length%32)/QUAD = 1
	 *         so:
	 *                 penultimate_word_clocks = 32-BITS_IN_BYTE
	 *                 last_word_clocks = (buffer_length%32)/QUAD + BITS_IN_BYTE
	 *                 last_word = penultimate_word>>24 | last_word<<8
	 */
	uint8_t last_word_clocks;

	/** @brief  Amount of clock pulses for penultimate word.
	 *          For more info see last_word_clocks.
	 */
	uint8_t penultimate_word_clocks;

	/** @brief Value of last word.
	 *         For more info see last_word_clocks.
	 */
	uint32_t last_word;

	/** @brief Function for writing to buffered out register. */
	hrt_fun_out_t fun_out;
} hrt_xfer_data_t;

/** @brief Hrt transfer parameters for WS2812. */
typedef struct {

	/** @brief Data for all transfer parts - WS2812 uses only HRT_FE_DATA */
	hrt_xfer_data_t xfer_data[HRT_FE_MAX];

	/** @brief Timer value, used for setting WS2812 bit timing frequency
	 *  This controls the VPR counter that generates precise WS2812 timing
	 */
	uint16_t counter_value;

	/** @brief WS2812 output pin - replaces CE functionality 
	 *  Index of the VIO pin used for WS2812 data output
	 */
	uint8_t ws2812_pin;

	/** @brief Tx mode mask for csr dir register (WS2812 output pin) */
	uint16_t tx_direction_mask;

} hrt_xfer_t;

/** @brief Write WS2812 data.
 *
 *  Function to be used to write data to WS2812 LED strips.
 *  Uses VPR (Vector Processing Unit) for precise timing control.
 *
 *  @param[in] hrt_xfer_params Hrt transfer parameters and WS2812 data.
 */
void hrt_write(volatile hrt_xfer_t *hrt_xfer_params);

#endif /* _HRT_H__ */
