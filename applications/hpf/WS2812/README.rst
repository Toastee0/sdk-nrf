```restructuredtext
.. _hpf_ws2812_example:

High-Performance Framework WS2812 driver
##########################################

.. contents::
    :local:
    :depth: 2

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

This application demonstrates an HPF-based driver for WS2812-compatible LED strips. The implementation
runs timing-critical LED signalling on the FLPR core (HRT) while exposing a simple API to the
application core for preparing and submitting LED frames.

Application overview
********************

The WS2812 HPF application is split into three logical parts:

* :ref:`The HPF application <hpf_mspi_example_api>` - Operates on the FLPR core and facilitates data transmission between the application core and the connected MSPI device.
* :ref:`The Hard Real Time (HRT) module <hpf_mspi_example_api>` - Runs on the FLPR core and facilitates data transmission between the FLPR core and the connected MSPI device.
  The module emulates the MSPI hardware peripheral on the FLPR core, managing real-time data transmission and ensuring precise timing and synchronization of data transfer operations.
* :ref:`The MSPI Zephyr driver <hpf_mspi_example_api>` - Operates on the application core and uses the Zephyr's scalable real-time operating system (RTOS) MSPI API for data and configuration transmission between the application and FLPR cores.

Scope
*****

This README focuses on the WS2812 HPF example in this folder. The driver demonstrates:

* Sending RGB (and optionally RGBW) frames to a WS2812-style LED strip with deterministic timing.
* A minimal API for queuing frames and querying status from the application core.
* Example wiring and boards configuration (see :file:`sample.yaml` and the ``boards/`` directory).

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

Design notes
************

- Timing accuracy and jitter are critical for WS2812 signalling. The HRT implementation runs on the
   FLPR core and uses dedicated timers to meet the protocol timing requirements.
- The application core passes frames via IPC (ICMsg) to the FLPR app. Large frames may be passed by
   reference or by copy depending on Kconfig options and memory constraints.

Building and running
********************

.. |application path| replace:: :file:`applications/hpf/ws2812`

.. include:: /includes/application_build_and_run.txt

To build and run the application, you must include code for both the application core and FLPR core.
The process involves building a test or user application that is using MSPI driver with the appropriate sysbuild configuration.

.. tabs::

   .. tab:: Building with tests

      You can use the tests listed in the :ref:`hpf_ws2812_example_testing` section.
      To build them, execute the following command in their respective directories:

      .. code-block:: console

         west build -p -b nrf54l15dk_nrf54l15_cpuapp

   .. tab:: Building with User Application

      Follow these steps to build with a user application:

      1. Enable the following Kconfig options:

         * ``SB_CONFIG_HPF`` - Enables the High-Performance Framework (HPF).
         * ``SB_CONFIG_HPF_MSPI`` - Integrates the HPF application image with the user application image.

      #. Disable the following Kconfig options:

         * ``SB_CONFIG_VPR_LAUNCHER`` - Disables the default VPR launcher image for the application core.
         * ``SB_CONFIG_PARTITION_MANAGER`` - Disables the :ref:`Partition Manager <partition_manager>`.

      #. Implement the business logic using the MSPI API with the HPF application.

      #. Add device tree nodes that describe the chips connected via MSPI (hpf_mspi).

      #. Compile your code.

.. _hpf_ws2812_example_testing:

Testing
*******

The following tests utilize the MSPI driver along with this application:
* ``nrf/tests/zephyr/drivers/mspi/api`` - reference implementation
* ``nrf/tests/zephyr/drivers/ws2812/api`` -ws2812 implementation
* ``nrf/tests/zephyr/drivers/flash/common`` -common flash tests

These tests report results through serial port (the USB debug port on the nFR54L15 DK).

Dependencies
************

* :file:`zephyr/doc/services/ipc/ipc_service` - Used for transferring data between application core and the FLPR core.
* `nrf HAL <nrfx API documentation_>`_ - Enables access to the VPR CSR registers for direct hardware control.
* :ref:`hpf_assembly_management` - Used to optimize performance-critical sections.

.. _hpf_mspi_example_api:

API documentation
*****************

Application uses the following API elements:

Zephyr driver
=============

* Header file: :file:`include/drivers/mspi/hpf_mspi.h` - reference
* Source file: :file:`drivers/mspi/mspi_hpf.c`-reference

* Header file: :file:`include/drivers/ws2812/hpf_ws2812.h` -ws2812 implementation
* Source file: :file:`drivers/ws2812/ws2812_hpf.c` -ws2812 implementation



FLPR application
================

* Source file: :file:`applications/hpf/mspi/src/main.c` -reference
* Source file: :file:`applications/hpf/ws2812/src/main.c` -ws2812 implementation



FLPR application HRT
====================

* Header file: :file:`applications/hpf/mspi/src/hrt/hrt.h` - reference
* Source file :file:`applications/hpf/mspi/src/hrt/hrt.c` - reference
* Assembly: :file:`applications/hpf/mspi/src/hrt/hrt-nrf54l15.s` -reference

* Header file: :file:`applications/hpf/ws2812/src/hrt/hrt.h`-ws2812 implementation
* Source file :file:`applications/hpf/ws2812/src/hrt/hrt.c` - ws2812 implementation
* Assembly: :file:`applications/hpf/ws2812/src/hrt/hrt-nrf54l15.s` -ws2812 implementation

