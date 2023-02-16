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
  { sent: <integer, seconds since epoch UTC or seconds since boot>,
    reading_interval: <integer, seconds between readings> }
```

where `reading_interval` is only sent if the device is a sensor (as opposed to only an actuator).
At startup, the device is usually enabled, that is, it will report readings if it is not told
otherwise.

## Observation message

Uploaded observation data (which need not be uploaded at the time of observation, but can be held
until a communication window is open) has the topic `snappy/reading/<device-class>/<device-id>` and
a JSON payload:

```
  { sent: <integer, seconds since epoch UTC or seconds since boot>,
    location: <string, location-id>,
    sequenceno: <nonnegative integer, reading sequence number since startup>
    ... }
```

The sequence number allows the server to organize the incoming data in case the time stamp is
missing.  It is reset to 0 every time the device is rebooted.  Messages are uploaded (and received)
in increasing sequence number order, though not all observations are necessarily uploaded.  A drop
in the sequence number hence indicates a reboot (but since not every observation is uploaded it is
not possible to detect all reboots using this fact).

The payload contains fields that represent the last valid readings of the sensors that are on the
device.  Each factor is reported by the device under the field name `F#<factor-name>` to avoid name
clashes.  See the FACTOR table of DATA-MODEL.md for the `<factor-name>` values.

TODO: One might ask whether `location` is not redundant, since the location of the device should be
recorded in the back-end databases, but since the user can change it independently of that it is OK
to send it, for the time being.

## Control message

AWS IoT can send a control message to the device via the topics `snappy/control/<device-id>`,
`snappy/control-class/<device-class>` and `snappy/control-all`, to which the device can subscribe.  The
message has a JSON payload with at least these fields:

```
  enable: <integer, 0 or 1, whether to enable or disable device, OPTIONAL>,
  reading_interval: <integer, positive number of seconds between readings, OPTIONAL>,
```

where `enable` controls whether the device performs and reports measurements, and `reading_interval`
controls how often the measurements are taken.

## Command message

AWS IoT can send a command message to the device via the topic `snappy/command/<device-id>` (and
unlike control message, not to the class or to all devices) with a JSON payload.  There are not currently
anny command messages.
