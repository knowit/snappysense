-*- fill-column: 100 -*-

# FreeRTOS (and ESP32-IDF) based firmware for SnappySense

## Overview

**This is a work in progress.**

It ports the SnappySense firmware from the Arduino framework to almost-pure FreeRTOS, with the
ESP32-IDF framework only at the device interface level for now.  FreeRTOS+IDF is at a lower lower
level than Arduino but higher quality (as well as C-based).

I'm not using VS Code for this, just emacs; YMMV.

If the esp-idf SDK is installed in `~/esp/esp-idf` then remember to
```
  . ~/esp/esp-idf/export.sh
```
in your shell, then
```
  idf.py help
  idf.py build
  idf.py flash monitor
```
etc.

## Thoughts

For the FreeRTOS-based firmware I'm being more careful about how the device operates and more
constrained in how the firmware is constructed.  The Arduino firmware uses a fairly loose tasking
system for everything; the FreeRTOS firmware instead has a single main task that runs a state
machine that loops through a sleep-monitor-communicate-sleep-... cycle.  There are a couple of
helper tasks but these are not strictly necessary.

Also, the FreeRTOS-based firmware is more careful about what it reports, and much more careful about
handling errors to avoid corrupted or meaningless data.

Finally, I want the FreeRTOS-based firmware to be non-polling and power-usage-friendly.  This
includes input coming from the serial line, the network, buttons, the motion sensor, and other
infrequent "peripherals".

There are relatively few and light dependencies on ESP32-IDF so far.  I expect that it would be
straightforward to move much of the code to an STM32 system, with HAL easily taking the place of
ESP32-IDF in many places.  Of course for higher-level services (HTTP, TCP, etc) there might be more
differences.

## TODO

### In progress

**Piezo player**: this needs to use the ledc library directly, not the ledc superstructure for Arduino
(which is not supported with ESP32-IDF).

**MEMS (sound sensor)**: this needs to program the ADC directly, not rely on the preprogramming of
the ADC by the Arduino framework.

### Unstarted high priority

These are all tasks that make use of some novel feature of IDF

**Time service**: Use SNTP somehow, it is supported by IDF.

**MQTT upload**: this needs to do something, using the standard IDF network stacks.

**NVRAM parameters**: the Arduino firmware supports lots of config variables (mostly related to
network access) that are stored in NVRAM.  On Arduino these are managed via the Parameters
abstraction; under IDF it'll be something else.  Note it is highly desirable that the storage
formats be compatible for the two firmware variants, or completely separate.

**Factory configuration**: The Arduino firmware supports a factory configuration setup where a
config can be uploaded to a web server, we need something similar.  It's a simple UI, it can be used
the same way for end-user configuration, and there's no reason to do something more complicated.

(If the web server *is* too complicated then listening on a TCP port should still be possible.)

**User configuration**: The Arduino firmware supports the user setting upp wifi (and some other
things) via a web page hosted by the device; we need something similar.
