/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "backend.h"
#include <zephyr/drivers/mbox.h>

static const struct mbox_dt_spec rx_channel = MBOX_DT_SPEC_GET(DT_PATH(mbox_consumer), rx);
static backend_callback_t cbck;

/**
 * @brief Callback function for when a message is received from the mailbox
 * @param instance Pointer to the mailbox device instance
 * @param channel The MBOX channel ID
 * @param user_data Unused in simplified implementation
 * @param msg_data Pointer to the received MBOX message with data
 *
 * This function processes WS2812 packets received directly via MBOX data transfer.
 * Based on the Zephyr mbox_data sample pattern for reliable data handling.
 */
static void mbox_callback(const struct device *instance, uint32_t channel, void *user_data,
			  struct mbox_msg *msg_data)
{
	/* Check for valid message data */
	if (!msg_data || !msg_data->data || msg_data->size < sizeof(hpf_ws2812_data_packet_t)) {
		return;
	}

	/* Extract packet directly from MBOX message - no shared memory needed */
	hpf_ws2812_data_packet_t *packet = (hpf_ws2812_data_packet_t *)msg_data->data;

	/* Process the WS2812 command immediately */
	cbck(packet);

	/* No complex locking needed - MBOX provides the synchronization */
}

/**
 * @brief Initialize the mailbox driver.
 *
 * This function sets up the mailbox receive channel with the callback
 * function for direct data transfer (no shared memory required).
 *
 * @return 0 on success, negative error code on failure.
 */
static int mbox_init(void *unused)
{
	int ret;

	/* Register the callback function for the mailbox receive channel */
	ret = mbox_register_callback_dt(&rx_channel, mbox_callback, NULL);
	if (ret < 0) {
		return ret;
	}

	/* Enable the mailbox receive channel */
	return mbox_set_enabled_dt(&rx_channel, true);
}

int backend_init(backend_callback_t callback)
{
	cbck = callback;

	/* Initialize MBOX - no shared memory needed, data passed directly in messages */
	return mbox_init(NULL);
}
