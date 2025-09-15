# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_WS2812_NRF54L15 ws2812_nrf54l15.c)
zephyr_library_include_directories_ifdef(CONFIG_WS2812_NRF54L15 ${CMAKE_CURRENT_SOURCE_DIR}/../../../)