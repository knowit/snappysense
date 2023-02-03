# Design notes for IDF-based firmware

## UX

There are two modes, "monitoring" which is a low-power mode suitable for running off battery, and
"slideshow", which requires connected power.

In "monitoring" mode the device sleeps mostly, and wakes up every so often (maybe hourly) to read
sensors and report readings.

In "slideshow" mode the device is always on, reads sensors frequently, and displays them on the
screen.

The device starts up in the mode it was last switched to.  The mode name is printed on the screen on
startup.

If the device is sleeping, it can be woken by pressing the button once.

When the device is not sleeping, a mode switch can be performed by pressing the button once.  The
new mode name is printed on the screen.  The mode is saved to nvram.

When the device is not sleeping, it can be forced into user configuration mode by long-pressing on
the button (at least 2s).  When user configuration mode is entered it is shown on the screen.  In
this mode, the device runs a web server that allows it to be configured in simple ways.  To leave
configuration mode, long-press the button again.

## Sensor logic

For the SEN0500, we can get instantaneous readings of the following at any time:

- temperature
- humidity
- uv intensity
- luminous intensity
- atmospheric pressure

For the SEN0514, we can get instantaneous readings of the following, ditto:

- air quality index (not clear what this is or means)
- total volatile organic compounds
- co2

For the former two sensors, we can make these readings when we want to, and we can (within reason)
put the device to sleep inbetween readings.  There does however seem to be a fair burn-in period for
the sensors, so *WHEN THE DEVICE IS POWERED ON OR BROUGHT BACK FROM SLEEP, IT MUST STAY ON FOR A FEW
MINUTES*.

For the PIR and MEMS, we can't really make instantaneous readings, because they can be wildly
misleading (notably, no movement can be detected even when there's lots of activity, and the sound
detected can be completely wrong).  Instead we must run detection over some time to get meaningful
readings from these.

For the PIR, we can enable interrupts for the monitoring window; any interrupt means there's
movement.

For the MEMS, we need to figure out what its data mean, but maybe high-frequency monitoring in
several 1s window is OK.  We probably want to map various ranges of output to LOW, MEDIUM, HIGH
noise.  But do we get amplitude (interesting probably) or frequency (not probably)?

There's also the matter of WIFI interacting poorly with the temperature sensor (to be verified).

I think that the "monitoring" mode therefore works like this:

- when the device first starts up it tries to configure itself
  - system time
- the device sleeps
- the device wakes up in response to a watchdog timer or a button press
- the device powers up its peripherals and starts taking readings to warm them up
- there's probably a burn-in period of about a minute, followed by a monitoring window
- during the monitoring window
  - instantaneous values are read multiple times and in the end, integrated
  - continuous values (movement, sound) are read continuously and some determination is made about movement and sound level
- when the monitoring window is closed
  - we connect to wifi if possible
    - if time was not configured previously, we try to configure time
    - we mqtt upload results and get commands
    - the wifi window stays open for at least a minute in order to let mqtt responses arrive, maybe longer
    - we then take wifi down
- the device goes to sleep

Here "sleep" can be deep sleep or just power-down-peripherals.

