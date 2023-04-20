-*- fill-column: 100 -*-

# Post-mortem (2023-04-20)

## Generality and use case issues

SnappySense has been great for playing around with hardware and sensor tech and how the sensors
interact with the cloud, but in general it has suffered from a problem of generality:

* It can in principle run on battery power, but the motion and sound sensors need to be powered on
  somewhat continously to be useful, so the battery won't last long in that case.

* The speaker/buzzer is probably most useful for distress signals, but without a battery level
  sensor there's not much to be distressed about ([WiFi, maybe](https://www.youtube.com/watch?v=rzOKkt8yOFw)).
  
* It can measure lots of things, but it is not known how useful some of them are (air pressure) and
  also probably the case that some of them are hard to measure accurately (temperature) and some of
  the measurements are very iffy (eCO2 is apparently not a good approximation to real CO2, and the
  altitude measurement is worthless because it's just a linear function of air pressure).

While it appears that there is a clear use case for the device ("inneklima") it's not actually clear
what that means.

Even if the CO2 measurement was of good quality
[it may not mean what you think](https://www.theatlantic.com/health/archive/2023/02/carbon-dioxide-monitor-indoor-air-pollution-gas-stoves/672923/).
Whether something is too cold or too warm or just right is a personal preference (ask my wife, with
whom I share a sweltering home office), and the same is true for light.

It was also always going to be questionable whether recording sensor data and aggregating them in
the cloud was going to be a good idea.  In particular it is questionable whether employees want it
to be recorded whether they are present in their home offices at particular times.  It turns out it is
not sufficient to turn off the collection of, say, the motion and noise sensors.  In practice,
temperature, light and air quality are all proxies for the presence of people, especially in the
colder and darker parts of the year.  Sharp peaks in humidity also: these are mostly indicative of
people showering or doing laundry, not of a poor indoor climate per se.

All these issues taken together suggest that the use case is not very well thought out.

As a result of the device generality and the vague use case, development of SnappySense has tended to focus on
exploring the hardware, not producing an end-user unit with a crisply defined usage area.

## Technical issues

There were more or less obscure problems that took a long time to track down and that were either
expensive to solve or not possible to solve.

* The AIR sensor interacted with the battery charger: The on-module battery charger without a
  battery attached would pull the voltage levels down to 2.6V periodically, this would confuse the
  sensor, which would not operate under that condition; it needs stable 3.3V power.
  
* The AIR sensor needed specific signalling to reset itself: When disabling bus power, it was
  necessary to pull the I2C bus down (not just set it to zero) in order for the sensor to register the
  reset condition and to come up in a known state when re-enabling power.

* The sensors can take some time to warm up to reach a reliable state, requiring the device to be
  "on" for a much longer time than one would think necessary.  This affects battery life too.
  
* Data reading needs to be performed carefully: noise and motion are not instantaneous quantities
  but values computed from sampling over time.

* The temperature sensor is very sensitive to heat coming off the device, either directly from the
  CPU or from the copper plane on the PCB.  We learned to control heat to some extent by powering
  down much of the device between monitoring sessions, and have had to experiment with air flow and
  different PCB layout.  This work may not be completed yet.

The Arduino framework is nice for fast prototyping but hostile to features required for production.

* It is not out-of-memory safe (the String type is particularly bad in this regard)

* It drops error codes on the floor in some cases (and in general the quality of the Arduino
  libraries seems pretty iffy)

* It has blocking loops inside libraries (eg waiting for a network to connect), preventing smooth
  operation of the rest of the system

* It does not have a notion of low-power modes and it is hard to make these work (though the
  deep-sleep mode does look like it can be made to work)

* It has subtle bugs that will manifest after running for 49 days (the millisecond counter
  overflows)

* It is undocumented to a surprising degree.

Some time was also lost reinventing the wheel by creating a tasking system on top of the Arduino
framework rather than recognizing that the framework can be bypassed and that the FreeRTOS tasking
and timer system can be used directly to better effect.

In summary, for a new project it probably makes sense to start out with Arduino for quick
prototyping, but for production one would go with FreeRTOS + ESP-IDF.  It is worth investigating
whether Arduino is actually a benefit even during prototyping.
