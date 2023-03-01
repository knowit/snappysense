-*- fill-column: 100 -*-

# SnappySense user manual (Arduino firmware + Hardware 1.0 and 1.1)

Updated 2023-03-01.

SnappySense measures environmental conditions and displays them on its little screen and/or uploads
them to a server for storage and future processing.

## Placement

Environmental sensors are on top of the device and the little plastic bulb is a movement sensor.
The device should be placed on a flat surface exposed to the environment upward, and with the
movement sensor pointing in the direction of movement in the room.

The device is affected by body heat and other heat sources so it measures room temperature most
meaningfully when placed a little ways away from such sources.

## Network configuration

The device can be configured with the name of a local WiFi network and a password for connecting to
it.  Enterprise networks and captive portals are not currently supported.

First switch the device to _Access point mode_ by pressing and holding the BTN1 button on the device
front for at least three seconds.  The screen will show the name of a network (`snp_A_B_no_C_cfg`
where A, B, and C depend on the device) and an IP address `192.168.X.Y` (where X and Y can vary).

Next, connect your laptop or phone to the network (ignore error messages) and open a browser to
`http://192.168.X.Y` (not https).  You'll be presented with a form.  Fill this form out and submit
it.

Finally, reboot: press and hold the BTN1 button for another three seconds, or push the reset button on
the cpu, or disconnect and reconnect power.

## Startup

When USB power or a battery with a charge is connected, the device boots up.  It first displays a
version message.  The last line of this should have a date and say "Arduino"; if it does not, you
are reading the wrong manual.  The date is the date the firmware was created.

## Day-to-day use

During normal operation the device has two major modes: _slideshow_ mode (really this is _demo
mode_) and _monitoring_ mode.

In slideshow mode, SnappySense is on continually, reads the environment frequently, and displays a
repeating slideshow of the readings on its little screen.  It uploads observations somewhat
frequently.  In this mode, SnappySense needs to be on an external power supply.

In monitoring mode, SnappySense is usually in a low-power state.  It wakes up from time to time to
read the environment and display the slideshow for a few cycles, before going to sleep again.  It
uploads observations occasionally.  In this mode, SnappySense can run on battery power for some
time, currently 24-36 hours.

The device always starts up in slideshow mode.  When the device is on, the user can switch between
slideshow mode and monitoring modes by pressing the BTN1 button on the device front once.  The
screen displays the mode being switched to.

When the device is sleeping, a BTN1 button press will wake it but not switch mode; the display will
show the current mode.

When SnappySense is appropriately configured, the observations are uploaded to AWS IoT for
processing and storage.  If uploading is not possible because no network is reachable, then
observations are held for a time but are gradually discarded as memory fills up.  The ones still in
memory will be uploaded if the network becomes available.

## Bugs and open issues of note

The temperature readings, especially in slideshow mode, tend to be several degrees on the high side.

The quality of the other sensors is fairly uncertain.

The sound sensor does not work on v1.0 devices.

The v1.0 and v1.1 devices must have a battery connected even if USB power is connected, due to a
hardware bug.

The device can run on USB power or battery, but it is not currently able to detect whether it has an
external power source or not.

The power consumption in battery mode is much too high at present.

## Factory configuration and other settings

The device also has a "factory" configuration containing server keys and so on, normally this is not
for the user to change but it may be interesting to inspect it.  When the device is in access point
mode and your laptop is connected to that access point, try

```
   curl http://192.168.X.Y/show
```

For more information, see [../CONFIG.md](../CONFIG.md).

