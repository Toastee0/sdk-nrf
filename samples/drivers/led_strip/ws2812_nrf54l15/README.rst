# WS2812 LED Strip Sample for nRF54L15

This sample demonstrates the WS2812 LED strip driver for nRF54L15 using the FLPR coprocessor for deterministic timing.

## Overview

The WS2812 driver leverages the nRF54L15's FLPR (Fast Low Power Processor) coprocessor to provide precise timing control for WS2812 LED strips. The ARM Cortex-M33 core handles the application logic and LED effects, while the RISC-V FLPR core manages the low-level timing-critical WS2812 protocol.

## Hardware Requirements

- nRF54L15 DK or compatible board
- WS2812 LED strip
- Connection wire from P1.06 to LED strip data input
- 5V power supply for LED strip (if using more than a few LEDs)

## Wiring

```
nRF54L15 DK    WS2812 Strip
P1.06     -->  Data Input
GND       -->  Ground
5V        -->  VCC (external supply recommended)
```

## Building and Running

```bash
west build -b nrf54l15dk_nrf54l15_cpuapp
west flash
```

## Features

- Standard Zephyr LED strip API compatibility
- Hardware-accelerated timing via FLPR coprocessor
- Support for up to 1024 pixels
- Minimal CPU overhead on main core
- Simple rainbow effect demonstration

## Configuration

The sample is configured for a 64-pixel LED strip on pin P1.06. To change:

1. Edit the overlay file to modify `chain-length` and `gpios` properties
2. Rebuild and flash

## Performance

The FLPR coprocessor provides deterministic timing with ±150ns accuracy, ensuring reliable WS2812 protocol compliance even under system load.