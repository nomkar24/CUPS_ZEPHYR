![alt text](image-1.png)

# Zephyr CUPS and PAPPL Print Server

This repository contains an embedded print server implementation running on Zephyr RTOS. It integrates CUPS (Common UNIX Printing System) and PAPPL (Printer Application Framework) for Espressif systems (ESP32/ESP32-S3) with PSRAM support.

## Features

* **Zephyr RTOS Integration**: Built as a Zephyr application using Kconfig and the CMake build system.
* **Apple Raster and PWG Raster Support**: Processes uncompressed Apple Raster (image/urf) and PWG Raster streams. The system is configured to disable direct PDF processing to force rasterization on the client device (macOS or iOS), reducing memory overhead on the microcontroller.
* **USB Host Stack**: Queries configuration and interface descriptors of connected USB printers using Espressif's USB host driver shims.
* **Network Discovery**: Advertises print services on the local network via mDNS and DNS-SD over Wi-Fi.
* **External Heap Allocation**: Uses SPIRAM/PSRAM to allocate buffers for print queues and raster stream processing.

## Repository Structure

* `boards/` - Hardware configuration files and device tree overlays for ESP32-S3.
* `include/` - FreeRTOS compatibility shims and custom headers.
* `tests/` - Test suites and application code for PAPPL, libcups, and zlib.
* `CMakeLists.txt` - Project compilation and linking configuration.
* `prj.conf` - Base configuration defining network buffers, POSIX shims, and mbedTLS options.
* `west.yml` - West manifest containing upstream dependencies and repository references.
* `user-mbedtls.h` - Customized mbedTLS header config for embedded targets.

## Dependencies

The project workspace is managed using West and retrieves the following dependencies:
* Zephyr RTOS
* zlib
* pdfio
* libcups (Zephyr port)
* pappl_zephyr (Zephyr port)

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

Compile and flash the firmware to your target board:

```bash
west build -b esp32s3_devkitc_procpu
west flash
```

## Technical Overview

### Print Pipeline
The application uses the `pwg_common-300dpi-600dpi-srgb_8` driver profile. This profile signals client devices to perform local rendering. The client transmits the pre-rasterized data stream via the Internet Printing Protocol (IPP), and the print server pipes it directly to the printer over USB with minimal internal buffering.

### Heap Wrapper and POSIX Shims
Because CUPS and PAPPL require POSIX threading and dynamic allocations, the build system wraps memory management functions (such as `shared_multi_heap_alloc`) and maps standard POSIX APIs to Zephyr's POSIX subsystem.