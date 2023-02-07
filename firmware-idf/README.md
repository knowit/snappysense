# ESP32-IDF based firmware for SnappySense

## Overview

This is a work in progress.  It ports the SnappySense firmware from the Arduino framework to the
ESP32-IDF framework, which is lower level, higher quality, and C-based.

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
``
etc.

## Thoughts

For the IDF firmware I'm being more careful about how the device operates and more constrained in
how the firmware is constructed.  The Arduino firmware uses a fairly loose tasking system for
everything; the IDF firmware instead has a single main task that runs a state machine that loops
through a sleep-monitor-communicate-sleep-... cycle.

Also, the IDF firmware is more careful about what it reports, and much more careful about handling
errors to avoid corrupted or meaningless data.

Finally, I want the IDF firmware to be non-polling and power-usage-friendly.  This includes input
coming from the serial line, the network, buttons, the motion sensor, and other infrequent
"peripherals".

## TODO

### In progress

**Piezo player**: this needs to use the ledc library directly, not the ledc superstructure for Arduino
(which is not supported with ESP32-IDF).

**Air sensor**: this has been implemented but is untested as of yet since it requires the v1.1 device to
run off battery power, and I can't find the batteries...

### Unstarted high priority

These are all tasks that make use of some novel feature of IDF

**Time service**: Use SNTP somehow, it is supported by IDF.

**MQTT upload**: this needs to do something, using the standard IDF network stacks.

**NVRAM parameters**: the Arduino firmware supports lots of config variables (mostly related to
network access) that are stored in NVRAM.  On Arduino these are managed via the Parameters
abstraction; under IDF it'll be something else.  Note it is highly desirable that the storage
formats be compatible for the two firmware variants, or completely separate.

**Factory configuration**: The Arduino firmware supports a factory configuration setup where a
config can be uploaded over the serial line, we need something similar.

**User configuration**: The Arduino firmware supports the user setting upp wifi (and some other
things) via a web page hosted by the device; we need something similar.
