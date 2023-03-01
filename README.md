-*- fill-column: 100 -*-

# SnappySense third-generation prototype, 2022/2023

[This repo](https://github.com/knowit/snappysense) holds the hardware designs, firmware source code,
design documents, and backend code for KnowIt ObjectNet's _SnappySense_ environmental sensor device
and its AWS IoT based backend.

End users want to go directly to the user manual: [firmware-arduino/MANUAL.md](firmware-arduino/MANUAL.md).

Developers can start looking in [BACKGROUND.md](BACKGROUND.md) for background, requirements, and
that type of discussion.

## Device

The v1.0.0 device has an Espressif ESP32 module-on-a-chip and a number of environmental sensors
(temperature, humidity, air quality, and other things - see `firmware-arduino/src/sensor.h`), as
well as a small OLED display.

The v1.1.0 device additionally has a small speaker.  Note the pinout for v1.1 differs from v1.0 in
some respects, be sure to use the correct firmware.

The standard v1.x firmware has FreeRTOS at the base and the Arduino framework layered on top and is
written in C++, see `firmware-arduino/src`.

### Design documents

The device user experience during setup and use is described in the user manual:
[firmware-arduino/MANUAL.md](firmware-arduino/MANUAL.md).

The firmware architecture is documented at the beginning of `firmware-arduino/src/main.cpp`, with
pointers to other files.

The device architecture, as seen by software (peripheral addresses, MCU pins, etc), is documented in
`firmware-arduino/src/device.cpp`.

### Device schematics and data sheets

Device schematics and data sheets will appear in this repo eventually (Issue #31).  See `hardware/`.

### Firmware source code and development

For the Arduino firmware we use Visual Studio Code for the development environment.  Before you
build the first time, you will need to install the PlatformIO extension in VSCode, see
[platformio.org](https://platformio.org) for pointers.

Coding standards are documented at the beginning of `firmware-arduino/src/main.h`.

#### Developer builds

Source code for the Arduino-based firmware is in `firmware-arduino/src/`, start with
`firmware-arduino/src/main.cpp` and `firmware-arduino/src/main.h`.

Enable `DEVELOPMENT` in `firmware-arduino/src/main.h` to make it easier to develop and test.

Make a copy of `firmware-arduino/src/development_config_template.txt` as
`firmware-arduino/src/development_config.h` and make modifications in that copy for your local
setup, as this file is required to provide default settings.

#### Release builds

To make a release build:

* Be sure to disable `SNAPPY_DEVELOPMENT` in `firmware-arduino/src/main.h`.
* Adjust any other switches in `firmware-arduino/src/main.h` that might feel like they are
  development-only.
* Pay attention to warnings during build, as some switches that should not be on during release
  cause warnings to be printed if they are enabled.

### ESP32-IDF based firmware

Experimental code for firmware based directly on the esp32-idf framework and FreeRTOS (lower level
than Arduino but better engineered) is in `firmware-idf/`.

## AWS IoT backend and frontend

Work in progress.  See documents in `aws-iot-backend` for more.
