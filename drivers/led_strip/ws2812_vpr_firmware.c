/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WS2812 FLPR VPR Core Implementation for nRF54L15
 *
 * Simple RISC-V VPR coprocessor firmware for deterministic WS2812 LED strip
 * control with precise GPIO timing.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../ws2812_ipc_protocol.h"

/* nRF54L15 Hardware Register Definitions */
#define NRF_GPIO0_BASE          0x40842500
#define NRF_TIMER130_BASE       0x40C85000
#define NRF_MBOX130_BASE        0x40C8C000

/* GPIO Register Offsets */
#define GPIO_OUT_OFFSET         0x004
#define GPIO_OUTSET_OFFSET      0x008
#define GPIO_OUTCLR_OFFSET      0x00C
#define GPIO_DIR_OFFSET         0x014
#define GPIO_DIRSET_OFFSET      0x018

/* MBOX Register Offsets */
#define MBOX_EVENTS_RECEIVE_OFFSET  0x100
#define MBOX_TASKS_SEND_OFFSET      0x000
#define MBOX_DATA_OFFSET            0x400

/* Hardware register access macros */
#define REG32(addr)             (*(volatile uint32_t *)(addr))
#define GPIO_REG(offset)        REG32(NRF_GPIO0_BASE + (offset))
#define MBOX_REG(offset)        REG32(NRF_MBOX130_BASE + (offset))

/* WS2812 timing constants for 128 MHz FLPR */
#define T0H_CYCLES    45     /* 350ns high time for '0' bit */
#define T0L_CYCLES    83     /* 650ns low time for '0' bit */
#define T1H_CYCLES    90     /* 700ns high time for '1' bit */
#define T1L_CYCLES    38     /* 300ns low time for '1' bit */
#define RESET_CYCLES  6400   /* 50µs reset pulse */

/* Global state */
static uint32_t g_pin_mask = 0;
static volatile struct ws2812_shared_buffer *g_shared_buffer = NULL;

/**
 * @brief Precise cycle delay using inline assembly
 */
static inline void delay_cycles(uint32_t cycles)
{
    /* Simple cycle-accurate delay loop */
    for (uint32_t i = 0; i < cycles; i++) {
        __asm__ volatile ("nop");
    }
}

/**
 * @brief Send a single bit to WS2812
 */
static void ws2812_send_bit(bool bit_value)
{
    if (bit_value) {
        /* Send '1' bit: 700ns high, 300ns low */
        GPIO_REG(GPIO_OUTSET_OFFSET) = g_pin_mask;
        delay_cycles(T1H_CYCLES);
        GPIO_REG(GPIO_OUTCLR_OFFSET) = g_pin_mask;
        delay_cycles(T1L_CYCLES);
    } else {
        /* Send '0' bit: 350ns high, 650ns low */
        GPIO_REG(GPIO_OUTSET_OFFSET) = g_pin_mask;
        delay_cycles(T0H_CYCLES);
        GPIO_REG(GPIO_OUTCLR_OFFSET) = g_pin_mask;
        delay_cycles(T0L_CYCLES);
    }
}

/**
 * @brief Send a single byte to WS2812 (MSB first)
 */
static void ws2812_send_byte(uint8_t byte)
{
    for (int i = 7; i >= 0; i--) {
        ws2812_send_bit((byte >> i) & 1);
    }
}

/**
 * @brief Send a complete pixel (GRB order)
 */
static void ws2812_send_pixel(const struct ws2812_pixel_data *pixel)
{
    ws2812_send_byte(pixel->green);
    ws2812_send_byte(pixel->red);
    ws2812_send_byte(pixel->blue);
}

/**
 * @brief Generate WS2812 reset pulse
 */
static void ws2812_reset_pulse(void)
{
    GPIO_REG(GPIO_OUTCLR_OFFSET) = g_pin_mask;
    delay_cycles(RESET_CYCLES);
}

/**
 * @brief Initialize GPIO for WS2812 output
 */
static void gpio_init(uint32_t pin_number)
{
    g_pin_mask = (1UL << pin_number);
    
    /* Configure pin as output, initially low */
    GPIO_REG(GPIO_DIRSET_OFFSET) = g_pin_mask;
    GPIO_REG(GPIO_OUTCLR_OFFSET) = g_pin_mask;
}

/**
 * @brief Send response to ARM core via MBOX
 */
static void send_status(uint32_t status)
{
    if (g_shared_buffer) {
        g_shared_buffer->status = status;
    }
    
    struct ws2812_mbox_msg response = {
        .type = status,
        .data = 0
    };
    
    MBOX_REG(MBOX_DATA_OFFSET) = *((uint32_t*)&response);
    MBOX_REG(MBOX_TASKS_SEND_OFFSET) = 1;
}

/**
 * @brief Process update command from ARM core
 */
static void process_update_command(void)
{
    if (!g_shared_buffer) {
        send_status(WS2812_STATUS_ERROR);
        return;
    }

    uint32_t pixel_count = g_shared_buffer->pixel_count;
    
    if (pixel_count > WS2812_MAX_PIXELS) {
        send_status(WS2812_STATUS_ERROR);
        return;
    }

    /* Update status to busy */
    g_shared_buffer->status = WS2812_STATUS_BUSY;

    /* Send all pixels */
    for (uint32_t i = 0; i < pixel_count; i++) {
        ws2812_send_pixel(&g_shared_buffer->pixels[i]);
    }

    /* Send reset pulse to latch data */
    ws2812_reset_pulse();

    /* Update complete */
    send_status(WS2812_STATUS_COMPLETE);
}

/**
 * @brief Main VPR firmware loop
 */
int main(void)
{
    uint32_t gpio_pin = 0;
    bool initialized = false;

    /* Main processing loop */
    while (1) {
        /* Check for MBOX message */
        if (MBOX_REG(MBOX_EVENTS_RECEIVE_OFFSET)) {
            /* Clear event */
            MBOX_REG(MBOX_EVENTS_RECEIVE_OFFSET) = 0;
            
            /* Read message */
            uint32_t msg_data = MBOX_REG(MBOX_DATA_OFFSET);
            struct ws2812_mbox_msg *msg = (struct ws2812_mbox_msg *)&msg_data;
            
            switch (msg->type) {
            case WS2812_MSG_INIT:
                gpio_pin = msg->data;
                gpio_init(gpio_pin);
                
                /* Initialize shared memory pointer */
                /* TODO: This should come from a proper shared memory address */
                /* For now, use a fixed address - this needs to be coordinated with ARM side */
                g_shared_buffer = (volatile struct ws2812_shared_buffer *)0x20000000;
                
                initialized = true;
                send_status(WS2812_STATUS_IDLE);
                break;
                
            case WS2812_MSG_UPDATE:
                if (initialized) {
                    process_update_command();
                } else {
                    send_status(WS2812_STATUS_ERROR);
                }
                break;
                
            case WS2812_MSG_SHUTDOWN:
                initialized = false;
                send_status(WS2812_STATUS_IDLE);
                break;
                
            default:
                send_status(WS2812_STATUS_ERROR);
                break;
            }
        }

        /* Check for shared memory command (polling fallback) */
        if (initialized && g_shared_buffer && 
            g_shared_buffer->command == WS2812_MSG_UPDATE &&
            g_shared_buffer->status == WS2812_STATUS_IDLE) {
            
            g_shared_buffer->command = 0;  /* Clear command */
            process_update_command();
        }

        /* Small delay to prevent busy waiting */
        delay_cycles(1000);
    }

    return 0;
}