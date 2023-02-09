-*- fill-column: 100 -*-

# SnappySense MQTT protocol

If configured with a network the device will connect to AWS IoT via MQTT every so often to upload
sensor data and receive commands.  The communication window remains open for a little while; data
and commands generated while the window is closed will be queued (modulo device limits) until the
window is open.  The device fleet is small and messages are fairly infrequent, so messages should be
sent with MQTT QoS=1 to avoid data loss.

## Startup message

At startup the device sends a message with topic `snappy/startup/<device-class>/<device-id>` and a
JSON payload:

```
  { time: <string, timestamp, OPTIONAL>,
    reading_interval: <integer, seconds between readings, OPTIONAL> }
```

where `reading_interval` is only sent if the device is a sensor (as opposed to only an actuator).
At startup, the device is usually enabled, that is, it will report readings if it is not told
otherwise.

A `timestamp` is a string derived from the ISO date format: `yyyy-mm-ddThh:ss/xxx` where the date
and time fields are obvious and `xxx` is a string from the set `{sun,mon,tue,wed,thu,fri,sat}`, ie,
the weekday name.  The field `time` can be absent from messages if the device is not configured for
time.  The time stamp normally represents the local time at the location of the device, if the
device is properly configured.

## Observation message

Uploaded observation data (which need not be uploaded at the time of observation, but can be held
until a communication window is open) has the topic `snappy/reading/<device-class>/<device-id>` and
a JSON payload:

```
  { time: <string, timestamp, OPTIONAL>,
    sequenceno: <nonnegative integer, reading sequence number since startup, REQUIRED>
    ... }
```

For `time`, se over.  The sequence number allows the server to organize the incoming data in case
the time stamp is missing.  It is reset to 0 every time the device is rebooted.  Messages are
uploaded (and received) in increasing sequence number order, though not all observations are
necessarily uploaded.  A drop in the sequence number hence indicates a reboot (but since not every
observation is uploaded it is not possible to detect all reboots using this fact).

The payload contains fields that represent the last valid readings of the sensors that are on the
device.  The defined fields are currently:

```
  temperature: <float, degrees celsius, OPTIONAL>
  humidity: <float, relative in interval 0..100, OPTIONAL>
  uv: <float, UV index 0..15, OPTIONAL>
  light: <float, luminous intensity in lux, range unclear, OPTIONAL>
  altitude: <float, meters relative to sea level, OPTIONAL>
  pressure: <integer, hecto-Pascals, OPTIONAL>
  airquality: <integer, 1..5, air quality index, OPTIONAL>
  airsensor: <integer, 0..3, air sensor status, OPTIONAL>
  tvoc: <integer, 0..65000, total volatile organic content in ppb, OPTIONAL>
  co2: <integer, 400..65000, equivalent co2 content in ppm, OPTIONAL>
  noise: <integer, range unclear, unit unclear, OPTIONAL>
  motion: <integer, 0 or 1, whether motion was detected during last measurement window, OPTIONAL>
  
```

Note that the possible sensor readings that may be reported are defined by the FACTOR table, see
DATA-MODEL.md.

## Control message

AWS IoT can send a control message to the device via the topics `snappy/control/<device-id>`,
`snappy/control/<device-class>` and `snappy/control/all`, to which the device can subscribe.  The
message has a JSON payload with at least these fields:

```
  enable: <integer, 0 or 1, whether to enable or disable device, OPTIONAL>,
  reading_interval: <integer, positive number of seconds between readings, OPTIONAL>,
```

where `enable` controls whether the device performs and reports measurements, and `reading_interval`
controls how often the measurements are taken.

(This protocol is buggy and pointlessly restricts the names of devices and classes.  More sensibly,
the topics would be `snappy/control-device/<device-id>`, `snappy/control-class/<device-class>`, and
`snappy/control-all`.)

## Command message

AWS IoT can send a command message to the device via the topic `snappy/command/<device-id>` (and
unlike control message, not to the class or to all devices) with a JSON payload of the following
form, other forms TBD:

```
  { actuator: <string>, reading: <value>, ideal: <value> }
```

The intent of this message is that the named actuator, if it exists, shall be triggered in such a
way that the difference beteen the `reading` and the `ideal` is decreaed.  (This is very
preliminary, but the effect can range from actually manipulating some environmental control, via
flashing a message to the operator prompting her to do something, to just dropping the message on
the floor.)
