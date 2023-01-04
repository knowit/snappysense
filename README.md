# snappysense

This is the firmware for KnowIt ObjectNet's "snappysense" environmental sensor device.

The v1.0 device has an esp32 MoC and a number of environmental sensors (temperature, humidity,
air quality, and other things - see src/sensor.h), as well as a small OLED display.

The firmware has FreeRTOS at the base and the Arduino framework layered on top.

We use Visual Studio Code for the development environment.  See platformio.org for some
starting pointers.  

(More information will appear by and by.)
