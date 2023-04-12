-*- fill-column: 100 -*-

# SnappySense power management (notes)

(Also see https://github.com/knowit/snappysense/issues/5.)

The ESP32 can save power in various ways: by reducing the CPU speed, by going into "light sleep"
mode, and by going into "deep sleep" mode.  In addition, SnappySense can conserve power by turning
off radios when they are not needed and disabling peripherals and busses when in the "sleep window".

Specifically, while SnappySense on the Arduino firmware can draw over 220 mA when the radios are on,
SnappySense on the IDF firmware draws only 6.4 mA in the light-sleep mode in the sleep window with
peripherals and busses powered down.

As a general observation, the Arduino firmware draws more power than the IDF firmware under normal
operation.  Part of the reason is likely that the IDF firmware has dynamic frequency scaling
enabled, so the device likely does not run on 240MHz most of the time, but this is known not to be
the full reason, because even with DFS disabled the IDF firmware drew about half the power of the
Arduino firmware.  More investigation needed, in particular, I'm not sure the IDF firmware runs the
CPU at 240MHz at all but I'm fairly sure the Arduino firmware does.

## Light sleep

### IDF firmware

Currently the IDF firmware has power management enabled, and SnappySense goes into light sleep mode
when it is in the sleep window, drawing 6.4 mA.  The rest of the time it saves power by using
dynamic frequency scaling, drawing 25-40 mA.  The code changes for this are modest.

The device enters light sleep mode when light sleep has been enabled and FreeRTOS notices that only
the idle task is running and that there is a longer time until the next timer interrupt than a set
limit (currently 3ms).  This logic is implemented in the Espressif additions to FreeRTOS.  It
requires some config changes, notably `CONFIG_PM_ENABLE`, `CONFIG_PM_DFS_INIT_AUTO`, and
`CONFIG_FREERTOS_USE_TICKLESS_IDLE`.

The main pain-point with light-sleep is actually that we want the user button (BTN1) to wake the
device when it is sleeping.  The device can be woken by a GPIO signal going high, but this is not
the logic we're normally using for the button (which is edge triggered, not level triggered).
Consequently, we reconfigure the button and enable light-sleep when we open the sleep window for the
device, and configure the button back and disable light-sleep when we close the sleep window.

As noted in comments in the code, it could in principle be in light-sleep mode the entire time, and
this would lead to further savings during normal operation (from 25-40 mA to 15-25 mA probably).
But we would have to find out how to change the button logic so that it both works as a wakeup
button in the sleep window and as a long-press / short-press mode switch button outside the sleep
window.  I've not bothered to do this yet; I think it should be doable though.

### Arduino firmware

The Arduino firmware does not have power management enabled (for reasons described below).  It draws
about 140 mA during monitoring operation with peripherals and busses enabled (but no radios), and
about 40 mA when in the sleep window.  With radios, the power draw can be up to 220 mA.

In order to use power management, the Arduino firmware must be configured to also use IDF, in order
for the ESP32 libraries' configuration to be changed properly.  Instead of `framework=arduino` it is
`framework=arduino,espidf` in `platformio.ini`.  Then there must be an `sdkconfig.defaults` file
that sets the variables we need, as for the IDF firmware (but also some others).  Finally, the
Arduino initialization code is apparently not run, so we need our own FreeRTOS `app_main()` to run
that and run the setup code and application loop.

There is **prototype** code for power management on the branch `arduino_low_power`, but it is not
complete, in particular, `app_main()` is not complete, it should closely follow the Arduino logic.
There may be other problems too.  When you run this firmware, there are various errors on the log
about uninitialized components, and there is line noise.

Note that PlatformIO / VS Code does not handle dependencies on `sdkconfig.defaults` well, if that
file is changed, you must manually nuke `sdkconfig.featheresp32` to force it to be regenerated.

## Deep sleep

For deep sleep, we configure the ULP with our current state and then effectively shut down the
device.  Waking up from deep sleep entails a reboot; during the boot we must recognize the saved
state (and presumably try to clear it out).  It is my understanding that disconnecting power also
clears the saved state, which would be a safe default.

The ULP has a timer to allow it to wake up.  I *believe* it also has the ability to react to GPIO
interrupts, which we'd want for the button press to wake the device.

Since deep sleep entails shutting the device down there's not any trickiness with shutting down the
"right" parts and then reinitializing them "correctly" on powerup; memory contents are lost, and we
reboot as we would after lost power or a reset keypress.

Deep sleep is not implemented in either firmware at this time.
