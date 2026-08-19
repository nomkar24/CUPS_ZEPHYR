<p align="left">
  <img src="./logo_2.svg" alt="Zephyr Logo" height="120" style="vertical-align: middle; margin-right: 25px;" />
  <img src="./logo_1.png" alt="Print Server Logo" height="120" style="vertical-align: middle;" />
</p>

<p align="left">
  <a href="https://www.zephyrproject.org/"><img src="https://img.shields.io/badge/Zephyr_RTOS-3.x-blue?style=flat-square&logo=zephyr&logoColor=white" alt="Zephyr RTOS"></a>
  <img src="https://img.shields.io/badge/Hardware-ESP32--S3_Only-red?style=flat-square&logo=espressif&logoColor=white" alt="Hardware Support">
  <img src="https://img.shields.io/badge/Language-C-green?style=flat-square" alt="Language">
  <img src="https://img.shields.io/badge/License-Apache_2.0-lightgrey?style=flat-square" alt="License">
</p>

# Zephyr CUPS and PAPPL Print Server

This repository contains an embedded print server implementation running on Zephyr RTOS. It integrates CUPS (Common UNIX Printing System) and PAPPL (Printer Application Framework) specifically for the ESP32-S3.

**Note**: This project is exclusively supported on the ESP32-S3. Standard ESP32 microcontrollers are unsupported due to the requirement of a native USB OTG controller (DWC OTG) for printer communication and the high RAM footprint requiring external PSRAM/SPIRAM.

## System Architecture

The print server acts as a bridge between network clients and physical USB printers:

* **Network Client Interface**: macOS, iOS, or other network clients discover the print server over Wi-Fi via mDNS/DNS-SD. Print jobs are sent using the Internet Printing Protocol (IPP).
* **Firmware Stack**: The incoming print stream is processed by the ESP32-S3 firmware, which links the `libcups` (CUPS API), `pappl_zephyr` (PAPPL print framework), and `zlib` (decompression engine) modules.
* **Printer Interface**: The processed raster print job is written directly to the physical printer using the ESP32-S3 native USB Host controller (DWC OTG driver).

## Documentation

For a detailed technical analysis, system design, and evaluation reports, see the [GSOC26 Documentation](./GSOC26_Documentation.pdf) file in this repository.

## Features

* **Zephyr RTOS Integration**: Built as a standard Zephyr application utilizing Kconfig and the CMake build system.
* **Apple Raster and PWG Raster Support**: Processes uncompressed Apple Raster (image/urf) and PWG Raster streams. The system disables direct PDF rendering to force rasterization on the client device (macOS or iOS), reducing RAM and CPU utilization on the microcontroller.
* **USB Host Stack**: Queries configuration and interface descriptors of connected USB printers using the native ESP32-S3 USB Host driver wrapper.
* **Network Discovery**: Automatically advertises IPP printing services on the local network via mDNS and DNS-SD over Wi-Fi.
* **External Heap Allocation**: Utilizes SPIRAM/PSRAM to allocate the large buffers required for print queues and raster image streams.

## Repository Structure

* `boards/` - Hardware configurations and devicetree overlays.
* `include/` - FreeRTOS compatibility shims and custom headers.
* `tests/` - Test suites and application code for PAPPL, libcups, and zlib.
* `CMakeLists.txt` - Project compilation and linking configuration.
* `prj.conf` - Base configuration defining network buffers, POSIX shims, and mbedTLS options.
* `west.yml` - West manifest containing upstream dependencies and repository references.
* `user-mbedtls.h` - Customized mbedTLS header configuration for embedded targets.

## Dependencies

The project workspace is managed using West and retrieves the following dependencies:

| Dependency | Repository | Role |
| :--- | :--- | :--- |
| Zephyr RTOS | [`zephyrproject-rtos/zephyr`](https://github.com/zephyrproject-rtos/zephyr) | Underlying RTOS kernel and drivers |
| zlib | [`nomkar24/zlib`](https://github.com/nomkar24/zlib) | Data decompression for print streams |
| pdfio | [`nomkar24/pdfio`](https://github.com/nomkar24/pdfio) | PDF processing utility engine |
| libcups | [`nomkar24/libcups_zephyr`](https://github.com/nomkar24/libcups_zephyr) | Core CUPS client and printer shims |
| pappl_zephyr | [`nomkar24/pappl_zephyr`](https://github.com/nomkar24/pappl_zephyr) | Ported PAPPL framework interface |

## Getting Started

### 1. Initialize Workspace

Initialize the workspace using West:

```bash
west init -m https://github.com/nomkar24/CUPS_ZEPHYR.git --mr main workspace
cd workspace
west update
```

### 2. Configure Wi-Fi Credentials

Specify your Wi-Fi credentials in `tests/pappl/wifi_config.h`:

```c
#define SSID "your-ssid"
#define PSK  "your-password"
```

### 3. Build and Flash

Compile and flash the firmware to the target ESP32-S3 board:

```bash
west build -b esp32s3_devkitc_procpu
west flash
```

## Acknowledgements

This project integrates and builds upon several open-source libraries:

* **[Zephyr Project](https://github.com/zephyrproject-rtos/zephyr)**: The real-time operating system kernel.
* **[PAPPL](https://github.com/michaelrsweet/pappl)**: The Printer Application Framework developed by Michael R Sweet.
* **[OpenPrinting CUPS](https://github.com/openprinting/cups)**: The Common UNIX Printing System libraries.
* **[pdfio](https://github.com/michaelrsweet/pdfio)**: The PDF processing library developed by Michael R Sweet.
* **[zlib](https://github.com/madler/zlib)**: The compression library developed by Jean-loup Gailly and Mark Adler.

## License

This project is licensed under the Apache License 2.0.