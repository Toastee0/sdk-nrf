/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WS2812 LED Strip Driver for nRF54L15 with FLPR Coprocessor
 *
 * Simple, reliable WS2812 LED control using the nRF54L15's FLPR coprocessor
 * for deterministic timing. Maintains compatibility with standard Zephyr
 * led_strip driver API.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "ws2812_ipc_protocol.h"

LOG_MODULE_REGISTER(ws2812_nrf54l15, CONFIG_LED_STRIP_LOG_LEVEL);

/* External VPR binary reference (linked as resource) */
extern const uint8_t ws2812_vpr_binary_start[];
extern const uint8_t ws2812_vpr_binary_end[];
extern const size_t ws2812_vpr_binary_size;

/* Driver configuration from device tree */
struct ws2812_nrf54l15_config {
    /** GPIO output pin specification */
    struct gpio_dt_spec output_pin;
    /** LED strip chain length */
    uint16_t chain_length;
    /** Shared memory address */
    uintptr_t shared_mem_addr;
    size_t shared_mem_size;
    /** MBOX channels for IPC */
    struct mbox_dt_spec mbox_tx;
    struct mbox_dt_spec mbox_rx;
};

/* Driver runtime data */
struct ws2812_nrf54l15_data {
    /** VPR initialization state */
    bool vpr_initialized;
    /** Synchronization */
    struct k_sem update_complete_sem;
    struct k_mutex update_mutex;
    /** Shared memory buffer */
    volatile struct ws2812_shared_buffer *shared_buffer;
    /** MBOX callback */
    mbox_callback_t mbox_callback;
};

/* Forward declarations */
static int ws2812_update_rgb(const struct device *dev, struct led_rgb *pixels, size_t num_pixels);
static size_t ws2812_length(const struct device *dev);

/* MBOX callback for VPR communication */
static void ws2812_mbox_callback(const struct device *dev, mbox_channel_id_t channel_id,
                                void *user_data, struct mbox_msg *msg)
{
    struct ws2812_nrf54l15_data *data = user_data;
    
    if (msg->data == WS2812_STATUS_COMPLETE) {
        k_sem_give(&data->update_complete_sem);
    }
}

/* Initialize VPR coprocessor */
static int ws2812_vpr_init(const struct device *dev)
{
    const struct ws2812_nrf54l15_config *config = dev->config;
    struct ws2812_nrf54l15_data *data = dev->data;
    int ret;

    /* TODO: Load VPR binary and start coprocessor */
    /* This would involve writing the VPR binary to coprocessor memory
     * and starting execution. For now, assume it's loaded externally. */

    /* Initialize shared memory */
    data->shared_buffer = (volatile struct ws2812_shared_buffer *)config->shared_mem_addr;
    data->shared_buffer->command = 0;
    data->shared_buffer->status = WS2812_STATUS_IDLE;
    data->shared_buffer->pixel_count = 0;
    data->shared_buffer->gpio_pin = config->output_pin.pin;

    /* Setup MBOX callback */
    data->mbox_callback = ws2812_mbox_callback;
    ret = mbox_register_callback(&config->mbox_rx, &data->mbox_callback, data);
    if (ret < 0) {
        LOG_ERR("Failed to register MBOX callback: %d", ret);
        return ret;
    }

    /* Send initialization message to VPR */
    struct ws2812_mbox_msg init_msg = {
        .type = WS2812_MSG_INIT,
        .data = config->output_pin.pin
    };
    
    ret = mbox_send(&config->mbox_tx, (struct mbox_msg *)&init_msg);
    if (ret < 0) {
        LOG_ERR("Failed to send init message to VPR: %d", ret);
        return ret;
    }

    data->vpr_initialized = true;
    LOG_INF("WS2812 VPR initialized for pin %d", config->output_pin.pin);
    
    return 0;
}

/* Update LED strip with RGB data */
static int ws2812_update_rgb(const struct device *dev, struct led_rgb *pixels, size_t num_pixels)
{
    const struct ws2812_nrf54l15_config *config = dev->config;
    struct ws2812_nrf54l15_data *data = dev->data;
    int ret;

    if (!data->vpr_initialized) {
        return -ENODEV;
    }

    if (num_pixels > config->chain_length) {
        return -EINVAL;
    }

    if (num_pixels > WS2812_MAX_PIXELS) {
        return -EINVAL;
    }

    k_mutex_lock(&data->update_mutex, K_FOREVER);

    /* Wait for VPR to be ready */
    while (data->shared_buffer->status == WS2812_STATUS_BUSY) {
        k_sleep(K_USEC(10));
    }

    /* Copy pixel data to shared memory (convert RGB to GRB) */
    data->shared_buffer->pixel_count = num_pixels;
    for (size_t i = 0; i < num_pixels; i++) {
        data->shared_buffer->pixels[i].red = pixels[i].r;
        data->shared_buffer->pixels[i].green = pixels[i].g;
        data->shared_buffer->pixels[i].blue = pixels[i].b;
    }

    /* Send update command to VPR */
    data->shared_buffer->command = WS2812_MSG_UPDATE;
    
    struct ws2812_mbox_msg update_msg = {
        .type = WS2812_MSG_UPDATE,
        .data = num_pixels
    };
    
    ret = mbox_send(&config->mbox_tx, (struct mbox_msg *)&update_msg);
    if (ret < 0) {
        LOG_ERR("Failed to send update message to VPR: %d", ret);
        k_mutex_unlock(&data->update_mutex);
        return ret;
    }

    /* Wait for completion */
    ret = k_sem_take(&data->update_complete_sem, K_MSEC(100));
    if (ret < 0) {
        LOG_ERR("VPR update timed out");
        k_mutex_unlock(&data->update_mutex);
        return -ETIMEDOUT;
    }

    k_mutex_unlock(&data->update_mutex);
    return 0;
}

/* Get LED strip length */
static size_t ws2812_length(const struct device *dev)
{
    const struct ws2812_nrf54l15_config *config = dev->config;
    return config->chain_length;
}

/* Driver initialization */
static int ws2812_init(const struct device *dev)
{
    const struct ws2812_nrf54l15_config *config = dev->config;
    struct ws2812_nrf54l15_data *data = dev->data;
    int ret;

    /* Initialize synchronization primitives */
    k_sem_init(&data->update_complete_sem, 0, 1);
    k_mutex_init(&data->update_mutex);

    /* Validate GPIO pin */
    if (!gpio_is_ready_dt(&config->output_pin)) {
        LOG_ERR("GPIO device not ready");
        return -ENODEV;
    }

    /* Initialize VPR coprocessor */
    ret = ws2812_vpr_init(dev);
    if (ret < 0) {
        return ret;
    }

    LOG_INF("WS2812 driver initialized for %d pixels on pin %d", 
            config->chain_length, config->output_pin.pin);

    return 0;
}

/* Driver API structure */
static const struct led_strip_driver_api ws2812_nrf54l15_api = {
    .update_rgb = ws2812_update_rgb,
    .update_channels = NULL,  /* Not implemented */
    .length = ws2812_length,
};

/* Device tree instantiation macro */
#define WS2812_NRF54L15_DEVICE(inst)                                                    \
    static struct ws2812_nrf54l15_data ws2812_nrf54l15_data_##inst;                   \
                                                                                        \
    static const struct ws2812_nrf54l15_config ws2812_nrf54l15_config_##inst = {      \
        .output_pin = GPIO_DT_SPEC_INST_GET(inst, gpios),                             \
        .chain_length = DT_INST_PROP(inst, chain_length),                             \
        .shared_mem_addr = DT_INST_PROP(inst, shared_mem_addr),                       \
        .shared_mem_size = DT_INST_PROP(inst, shared_mem_size),                       \
        .mbox_tx = MBOX_DT_SPEC_INST_GET(inst, tx),                                   \
        .mbox_rx = MBOX_DT_SPEC_INST_GET(inst, rx),                                   \
    };                                                                                  \
                                                                                        \
    DEVICE_DT_INST_DEFINE(inst, ws2812_init, NULL,                                    \
                          &ws2812_nrf54l15_data_##inst,                               \
                          &ws2812_nrf54l15_config_##inst,                             \
                          POST_KERNEL, CONFIG_LED_STRIP_INIT_PRIORITY,                \
                          &ws2812_nrf54l15_api);

DT_INST_FOREACH_STATUS_OKAY(WS2812_NRF54L15_DEVICE)