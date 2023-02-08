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

### Unstarted high priority

These are all tasks that make use of some novel feature of IDF

**Time service**: Use SNTP somehow, it is supported by IDF.

**MQTT upload**: this needs to do something, using the standard IDF network stacks.

**NVRAM parameters**: the Arduino firmware supports lots of config variables (mostly related to
network access) that are stored in NVRAM.  On Arduino these are managed via the Parameters
abstraction; under IDF it'll be something else.  Note it is highly desirable that the storage
formats be compatible for the two firmware variants, or completely separate.

**Factory configuration**: The Arduino firmware supports a factory configuration setup where a
config can be uploaded over the serial line, we need something similar.  We don't need to worry
about power consumption in this mode.  There are a couple of options:

- Use the serial line; this is probably simplest but it does require a PC with the necessary software
- Connect via i2c from another dedicated "programming" device; this is sort of sexy but complicated
- Have the device connect to a pre-configured access point and ask for its configuration
- Have the device erect an access point and upload the config via the web, this should not be hard and
  requires only a web browser or indeed a terminal and curl.
- As the previous one, but don't even listen for web traffic - just listen on a TCP port and receive
  some text data.  Now all we need is netcat.
- Upload a file to the device's file system, do not involve application logic.  If the device does not
  find the file it will show an error and hang.  This reduces the amount of logic on the device.
  NVRAM settings (for a few things) override or complement the settings in the file.  It looks like
  this can be accomplished using some combination of spiffsgen.py and parttool.py,
  https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/spiffs.html#spiffsgen.py
- NFC, bluetooth
- SD card

I probably favor the solution where the device either opens an AP or connects to a known AP in
config mode and then listens on a port for a config file, the necessary information can be displayed
on the screen.

Of course, *end-user* config also needs some kind of solution and that may be a web server :-/

The preconfigured AP can be provided by another ESP32... but then that one needs to generate data
and so on, and none of that will work out OK.

**User configuration**: The Arduino firmware supports the user setting upp wifi (and some other
things) via a web page hosted by the device; we need something similar.
