-*- fill-column: 100 -*-

# User experience, configuration, etc

(This is for the Arduino-based firmware only, for the time being.  The IDF-based firmware has its
own documentation.)

## Purpose and day-to-day use

SnappySense measures environmental conditions and displays them on its little screen and/or uploads
them to a server for storage and future processing.

The device has two major modes: _slideshow_ mode and _monitoring_ mode.

In slideshow mode (really a _demo_ mode), SnappySense is on continually, reads the environment
frequently, and displays a repeating slideshow of the readings on its little screen.  It uploads
obserationts somewhat frequently.  In this mode, SnappySense needs to be on an external power
supply.

In monitoring mode, SnappySense is usually in a low-power state.  It wakes up from time to time to
read the environment and display the slideshow for a few cycles, before going to sleep again.  It
uploads observations occasionally.  In this mode, SnappySense can run on battery power for some
time.

The device starts up in slideshow mode.  When the device is on, the user can switch between
slideshow mode and monitoring mode by pressing the button on the device front (labeled WAKE on v1.0
and BTN1 on v1.1).  The screen displays the mode being switched to.  When the device is sleeping, a
button press will wake it but not switch mode; the display will show the current mode.

When SnappySense is appropriately configured, the observations are uploaded to AWS IoT for
processing and storage.  If uploading is not possible because no network is reachable, then
observations are held for a time but are gradually discarded as memory fills up.  The ones still in
memory will be uploaded if the network becomes available.

The device can run on USB power or battery, but it is not currently able to detect whether it has an
external power source or not.

The v1.0 and v1.1 devices must have a battery connected even if USB power is connected, due to a
hardware bug.

## Access point mode

The SnappySense device can be switched into "access point mode" (AP mode), allowing it to be
configured and for its configuration to be queried.

To enter AP mode, press and hold the button on the front of SnappySense (labeled WAKE on v1.0 and
BTN1 on v1.1) for at least 3 seconds; the screen will indicate when the mode is switched.  When
SnappySense enters AP mode, the message on its little screen will show a network name and an IP
address on the form `192.168.x.y`.  If the device has not been given an access point name, it will
generate a random one.

### End-user configuration

In AP mode, the user can perform _end-user configuration_ by connecting to the named WiFi network
(ignore any errors you may see about the network not providing internet access).  Then in a browser
on the connected device, open a page at `http://192.168.x.y`.  (Be sure not to use `https` at this
time.)  You will see a form with configuration options.  Change what you want to change, then press
the SUBMIT button to store the options on SnappySense.

After configuration, either press the RESET button on the back of SnappySense or long-press the
WAKE/BTN1 button to restart the device with the new settings.

The configuration values are in two groups: the names and passwords for any networks it can connect
to for uploading data (up to three networks), and the name of the location the device is in.  The
location name is not quite arbitrary, it should be one that has been registered in the database on
AWS.  The name is used to help identify the data and will be unique and (usually) meaningful.

(TODO: It would be useful for this form to allow a selection of the factors being measured and
displayed.)

### Factory provisioning

In AP mode, the device similarly responds to factory configuration and query commands.  These are
more complicated, not usefully performed after initial setup, and described in CONFIG.md.
