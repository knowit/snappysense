# snappysense

This is the firmware for KnowIt ObjectNet's "snappysense" environmental sensor device.

The v1.0 device has an esp32 MoC and a number of environmental sensors (temperature, humidity,
air quality, and other things - see src/sensor.h), as well as a small OLED display.

The firmware has FreeRTOS at the base and the Arduino framework layered on top.

We use Visual Studio Code for the development environment.  Before you build the first time, you will need to:
* install the PlatformIO extension in Visual Studio Code, see platformio.org for pointers.  
* make a copy of src/client_config_template.txt as src/client_config.h and make modifications in that copy for your local setup.

## Guide to the source code

Generally, start with src/main.cpp and src/main.h.

(More information will appear by and by.)
