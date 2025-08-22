this project is an attempt to follow the template of the mspi driver's interface with zephyr and the hpf framework to create a working ws2182 driver for the hpf / FLPR core.

Progress so far:
first modified header
C:\ncs\v3.1.0-rc2\zephyr\include\zephyr\drivers\ws2812.h
second modified header
C:\ncs\v3.1.0-rc2\nrf\include\drivers\ws2812\hpf_ws2812.h
modified copy of mspi, changed to be for ws2812
C:\ncs\v3.1.0-rc2\nrf\applications\hpf\WS2812
