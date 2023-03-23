-*- fill-column: 100 -*-

# SnappySense MQTT protocol

If configured with a network the device will connect to an MQTT message broker every so often to upload
sensor data and receive commands.  The communication window remains open for a little while; data
and commands generated while the window is closed will be queued (modulo device limits) until the
window is open.  The device fleet is small and messages are fairly infrequent, so messages should be
sent with MQTT QoS=1 to avoid data loss.

Since the device class ID and device ID are in the message topics, it is best to keep these IDs very
simple.  Obviously avoid `/` in either ID, but for RabbitMQ it is also necessary to avoid `.`.
Probably both `_` and `-` are safe in addition to ASCII alphanumeric characters.

Note that the `version` fields all relate to the package they are part of, they are not necessarily
the same version.

## Startup message

At startup the device sends a message with topic `snappy/startup/<device-class>/<device-id>` and a
JSON payload:

```
  { version: <string, semver for this JSON package, currently 1.0.0>,
    sent: <integer, seconds since Posix epoch UTC>,
    interval: <integer, seconds between observations> }
```

where `interval` is only sent if the device is a sensor (as opposed to only an actuator).  At
startup, the device is usually enabled, that is, it will report observations if it is not told
otherwise.

See `firmware-arduino/src/mqtt.cpp` : `generate_startup_message()` for a definition of `version`.

## Observation message

Uploaded observation data (which need not be uploaded at the time of observation, but can be held
until a communication window is open) has the topic `snappy/observation/<device-class>/<device-id>` and
a JSON payload:

```
  { version: <string, semver for this JSON package, currently 1.0.0>,
    sent: <integer, seconds since Posix epoch UTC>,
    sequenceno: <nonnegative integer, observation sequence number since startup>
    ... }
```

The sequence number is reset to 0 every time the device is rebooted.  Messages are uploaded (and
received) in increasing sequence number order, though not all observations are necessarily uploaded.
A drop in the sequence number hence indicates a reboot, but since not every observation is uploaded
it is not possible to detect all reboots using this fact.  (The sequence number is probably
redundant and can be removed.  It allows the server to organize the incoming data in case the time
stamp is missing, but in recent firmware that will never be the case.  Hence version 1.0.0 defines
the sequence number field as optional.)

The payload contains fields that represent the last valid observations of the sensors that are on the
device.  Each factor is reported by the device under the field name `F#<factor-name>` to avoid name
clashes.  See the FACTOR table of DATA-MODEL.md for the `<factor-name>` values.

See `firmware-arduino/src/sensor.cpp` for a definition of `version`.

## Control message

The message broker (or really, code behind it that it routes messages to) can send a control message
to the device via the topics `snappy/control/<device-id>`, `snappy/control-class/<device-class>` and
`snappy/control-all`, to which the device can subscribe.  The message has a JSON payload with at
least these fields:

```
  version: <string, semver for this JSON package, currently 1.0.0>
  enable: <integer, 0 or 1, whether to enable or disable device, OPTIONAL>,
  interval: <integer, positive number of seconds between observations, OPTIONAL>,
```

where `enable` controls whether the device performs and reports measurements, and `interval`
controls how often the measurements are taken.

See `firmware-arduino/src/mqtt.cpp` : `mqtt_handle_message()` for a definition of `version`.

## Command message

The message broker (or really, code behind it that it routes messages to) can send a command message
to the device via the topic `snappy/command/<device-id>` (and unlike control message, not to the
class or to all devices) with a JSON payload.  There are not currently anny command messages.
