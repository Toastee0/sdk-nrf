/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "./backend/backend.h"
#include "./hrt/hrt.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <drivers/gpio/hpf_gpio.h>
#include <drivers/led_strip/hpf_ws2812.h>
#include <hal/nrf_vpr_csr.h>
#include <hal/nrf_vpr_csr_vio.h>
#include <hal/nrf_vpr_csr_vtim.h>
#include <haly/nrfy_gpio.h>

#define HRT_IRQ_PRIORITY          2
#define HRT_VEVIF_IDX_WS2812_REFRESH  17
#define HRT_VEVIF_IDX_WS2812_CLEAR    18

#define VEVIF_IRQN(vevif) VEVIF_IRQN_1(vevif)
#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

static nrf_gpio_pin_pull_t get_pull(gpio_flags_t flags)
{
	if (flags & GPIO_PULL_UP) {
		return NRF_GPIO_PIN_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		return NRF_GPIO_PIN_PULLDOWN;
	}

	return NRF_GPIO_PIN_NOPULL;
}

static int gpio_hpf_pin_configure(uint8_t port, uint16_t pin, uint32_t flags)
{
	/* FLPR VIO (VPR I/O) can only access GPIO Port 2 (P2.0-P2.10)
	 * VIO pin numbering: 4,0,1,3,2,5..10 maps to GPIO P2.0-P2.10
	 * IMPORTANT: XIAO P1.04 is NOT accessible from FLPR!
	 * User must select a Port 2 pin (e.g., P2.00-P2.10) for WS2812 output
	 */
	if (port != 2) {
		return -EINVAL;
	}

	uint32_t abs_pin = NRF_GPIO_PIN_MAP(port, pin);
	nrf_gpio_pin_pull_t pull = get_pull(flags);
	nrf_gpio_pin_drive_t drive;

	switch (flags & (NRF_GPIO_DRIVE_MSK | GPIO_OPEN_DRAIN)) {
	case NRF_GPIO_DRIVE_S0S1:
		drive = NRF_GPIO_PIN_S0S1;
		break;
	case NRF_GPIO_DRIVE_S0H1:
		drive = NRF_GPIO_PIN_S0H1;
		break;
	case NRF_GPIO_DRIVE_H0S1:
		drive = NRF_GPIO_PIN_H0S1;
		break;
	case NRF_GPIO_DRIVE_H0H1:
		drive = NRF_GPIO_PIN_H0H1;
		break;
	case NRF_GPIO_DRIVE_S0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_S0D1;
		break;
	case NRF_GPIO_DRIVE_H0 | GPIO_OPEN_DRAIN:
		drive = NRF_GPIO_PIN_H0D1;
		break;
	case NRF_GPIO_DRIVE_S1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0S1;
		break;
	case NRF_GPIO_DRIVE_H1 | GPIO_OPEN_SOURCE:
		drive = NRF_GPIO_PIN_D0H1;
		break;
	default:
		return -EINVAL;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs | (BIT(pin)));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		uint16_t outs = nrf_vpr_csr_vio_out_get();

		nrf_vpr_csr_vio_out_set(outs & ~(BIT(pin)));
	}

	nrf_gpio_pin_dir_t dir =
		(flags & GPIO_OUTPUT) ? NRF_GPIO_PIN_DIR_OUTPUT : NRF_GPIO_PIN_DIR_INPUT;
	nrf_gpio_pin_input_t input =
		(flags & GPIO_INPUT) ? NRF_GPIO_PIN_INPUT_CONNECT : NRF_GPIO_PIN_INPUT_DISCONNECT;

	/* Reconfigure the GPIO pin with the specified pull-up/pull-down configuration and drive
	 * strength.
	 */
	nrfy_gpio_reconfigure(abs_pin, &dir, &input, &pull, &drive, NULL);

	if (dir == NRF_GPIO_PIN_DIR_OUTPUT) {
		nrf_vpr_csr_vio_dir_set(nrf_vpr_csr_vio_dir_get() | (BIT(pin)));
	}

	/* Take control of the pin */
	nrfy_gpio_pin_control_select(abs_pin, NRF_GPIO_PIN_SEL_VPR);

	return 0;
}

/* Global state for WS2812 control */
static struct {
	uint32_t pin;
	uint8_t port;
	uint32_t num_leds;
	bool configured;
	hpf_ws2812_pixel_t pixel_buffer[HPF_WS2812_MAX_LEDS];
} ws2812_state = {
	.configured = false
};

void process_packet(hpf_ws2812_control_packet_t *control, hpf_ws2812_pixel_t *pixels)
{
	if (control->opcode != HPF_WS2812_UPDATE) {
		return;
	}

	/* Configure GPIO if this is the first update */
	if (!ws2812_state.configured || 
	    control->pin != ws2812_state.pin || 
	    control->port != ws2812_state.port) {
		
		uint32_t gpio_flags = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;
		gpio_hpf_pin_configure(control->port, control->pin, gpio_flags);
		
		ws2812_state.pin = control->pin;
		ws2812_state.port = control->port;
		ws2812_state.configured = true;
	}

	/* Copy pixel data to local buffer - disable interrupts to prevent race condition */
	if (control->num_leds > 0 && control->num_leds <= HPF_WS2812_MAX_LEDS) {
		uint32_t key = irq_lock();
		
		ws2812_state.num_leds = control->num_leds;
		for (uint32_t i = 0; i < control->num_leds; i++) {
			ws2812_state.pixel_buffer[i] = pixels[i];
		}
		
		irq_unlock(key);
		
		/* Trigger WS2812 pixel refresh via interrupt */
		nrf_vpr_clic_int_pending_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WS2812_REFRESH));
	}
}

#define HRT_CONNECT(vevif, handler)                                            \
	IRQ_DIRECT_CONNECT(vevif, HRT_IRQ_PRIORITY, handler, 0);               \
	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(vevif), true)


__attribute__ ((interrupt)) void hrt_handler_ws2812_refresh(void)
{
	/* Send pixel data to WS2812 strip using HRT function */
	hrt_ws2812_refresh((const uint8_t *)ws2812_state.pixel_buffer);
}

__attribute__ ((interrupt)) void hrt_handler_ws2812_clear(void)
{
	/* Send reset/latch pulse to WS2812 strip */
	hrt_ws2812_clear();
}

int main(void)
{
	int ret = 0;

	/* Initialize backend communication (ICMSG or MBOX) */
	ret = backend_init(process_packet);
	if (ret < 0) {
		return 0;
	}

	/* Set up timer for precise WS2812 timing */
	nrf_vpr_csr_vtim_simple_counter_set(0, 0);
	nrf_vpr_csr_vtim_count_mode_set(0, NRF_VPR_CSR_VTIM_COUNT_RELOAD);

	/* Connect WS2812 interrupt handlers */
	HRT_CONNECT(HRT_VEVIF_IDX_WS2812_REFRESH, hrt_handler_ws2812_refresh);
	HRT_CONNECT(HRT_VEVIF_IDX_WS2812_CLEAR, hrt_handler_ws2812_clear);

	/* Enable real-time peripherals */
	nrf_vpr_csr_rtperiph_enable_set(true);

	/* Main loop - wait for commands from ARM core */
	while (true) {
		k_cpu_idle();
	}

	return 0;
}
