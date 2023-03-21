-*- fill-column: 100 -*-

# Some opinions, 2023-03-09

## Thoughts

We need to decide what the device is for, *specifically*, because now it is very general and it is
hard to know what to optimize for.

Are we optimizing for power usage or data quality?  For the sweetness of the design or for the data
quality?  Are we requiring reasonably frequent data uploads or only uploads when the stars align?
Is the device a proof of concept to use as a calling card ("look what we've built") or something we
want to rely on for actually measuring environmental conditions?

- If it is for *monitoring the workplace environment* then
  - It needs to monitor sound pretty much continuously or at least very frequently, since sound is a
    major issue in the workplace
  - In turn this means that running on battery is really not going to work out very well
  - Even if we were only to monitor sound when movement is detected, and let a PIR interrupt on the
    ULP or similar battery-friendly device wake the main device on movement and start sound
    monitoring, the reality is that there will be movement during most of the workday.  One might
    just as easily always monitor sound from 7am - 7pm.

- If it is for producing *reliable* data then
  - The problems with the temperature sensor must be fixed
  - The problems with the AIR sensor must be fixed
  - We must face all the hard calibration and reliability problems and must be prepared to commit to
    statistical processing and smoothing of data and to investigate how data are made more reliable;
    for example, the sound sensor has been known to produce spikes that must probably be removed
    from the data
  - We must face all hard sampling frequency problems

In sum, a *workplace monitor* runs off the power grid, not off battery.

A *workplace monitor* also has issues with WiFi connectivity, since the workplaces sensibly will not
allow the device to connect to their internal networks (wireless or wired), and "open" networks
frequently are not actually open, but captive portals.  How do we solve this issue?

## Opinions

Most sensibly we turn this into a reliable workplace monitor (if we're not going to abandon it).

It runs primarily on connected power, not battery.

It monitors the environment often and produces high-resolutionj data.

Sensor reliability issues *must* be fixed:

- move the temperature sensor off the main board
- solve the problem with the power issues for the AIR sensor
- all sensors we use and report must be calibrated against reliable sensors

Data uploads happen when possible, over wifi as now, but the user may have to opt in to turning on a
mobile hotspot every so often or to move the device into the space of a network.  The device
probably wants UI (a button) to perform uploads.  This is very clunky.

There should either be battery backup so that the device can be moved to a spot where it can upload
data, or there should be a flash disk that it can store data into, until they can be uploaded.  This
is also very clunky.

On the front-end, there must be a dashboard to control the devices, specifically, to enable or
disable individual measurements and the device as a whole, to check and wipe data.

