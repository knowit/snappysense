# SnappySense manual (IDF firmware)

## Normal operation

### Mode switch (NYI)

The device starts in slideshow mode.  To switch to power-saving monitoring mode, press BTN1 once,
the screen will read "Monitoring".  To switch back, press BTN1 once.

### Network access

The device can operate standalone without WiFi access.  When in slideshow mode, it will show a
screen displaying "No WiFi" if there is no network connected (no access point found or no connection
maintained).  Data for upload will be held for a time, but not across restarts.

## User configuration (NYI)

Note, the system has to be factory configured before user configuration can take place.

User configuration can change WiFi access points and location ID.

To enter user configuration mode, press and hold BTN1 until the screen reads "User config" and the
name of a WiFi access point.  Connect to that access point and open a browser to the address
http://snappy.config.  A form comes up; it is self-explanatory.  Fill it out and submit it.

To exit access mode, press and hold BTN1 until the device leaves config mode (it will reboot),
or just press RESET.


## Factory configuration (NYI)

To enter factory configuration mode, press and hold BTN1, then press and release RESET.

In this mode the device will announce itself on the USB line and read a config from the USB line,
before saving that and restarting.