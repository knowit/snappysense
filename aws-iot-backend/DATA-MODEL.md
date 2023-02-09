-*- fill-column: 100 -*-

# SnappySense data model

For notes on the overall design, see design.md in this directory.

## Location

An entry in `LOCATION` represents a location where we can place sensors and actuators.  A location
records the devices in it and every device records its location, so the tables must be maintained
together.

Observe that the actuators incorporate data about desired settings for the location.  The `idealfn`
attribute is either an `ideal-id` or an `ideal-id` concatenated with constant parameters, separated
by slashes, see example below.

DynamoDB table name: `snappy_location`.  Primary key: `location`.

TODO: Even if this does not have a geolocation (though it could), it might usefully have a
time zone, so that computing eg "work time" vs "non-work time" is possible.

```
LOCATION
    location: <string: location-id>
    description: <string: human-readable, fairly specific>
    sensors: [<string: device-id> ...]
    actuators: [{factor: <string, factor-id>,
                 device: <string: device-id>,
                 idealfn: <string: ideal-id + parameters>}, ...}
    ... other fields, eg, geographic position, if we want
```

Example:
```
    name: "lars-t-hansen-hjemmekontor"
    description: "Gjemmekontoret til Lars T, dypt inne i Lillomarka"
    sensors: ["1", "2"]
    actuators: [{factor: "temperature", device:"1", ideal_fn: "work_temperature/21/19"}
                {factor: "humidity", device: "3", ideal_fn: "constant/50"}]
    earth: "@60.00438419,10.84708148,397.04017576a"
```

## Device

An entry in `DEVICE` represents a single device, which can be a sensor or an actuator or both.  The
default for `enabled` is 1.  The default for `reading_interval` is something sensible.

DynamoDB table name: `snappy_device`.  Primary key: `device`.  Sort key: `class`.

TODO: It's possible that the `reading_interval` on a device should be per factor and not for the
device as a whole, and that the default `reading_interval` for a factor should be stored in
`FACTOR`.

TODO: Not obvious that the sort key is necessary.

TODO: Booleans are a thing, `enabled` need not be an int.

```
DEVICE
    device: <string, device-id>
    class: <string, class-id>
    location: <string, location-id>
    enabled: <integer, 0 or 1>
    reading_interval: <positive integer, seconds>
    sensors: [<string: factor-id>, ...]
    actuators: [<string: factor-id>, ...]
```

The device-id has a format that describes it generally.  For example, the SnappySense devices have
IDs that are on the format `snp_x_y_no_z` where `x.y.0` is the hardware version and `z` is the
serial number of the device within that version.  These are AWS IoT device IDs.

Example:
```
    device: "snp_1_1_no_1",
    class: "snappysense",
    location: "lars-t-hansen-hjemmekontor",
    sensors: ["temperature","humidity","uv","light","pressure","altitude","airsensor",
	          "airquality","tvoc","co2","motion","noise"],
    actuators: ["temperature"]
```

## History

There is one entry in `HISTORY` for each device.  The entry holds the last readings from the device,
the last actions, and information about the time of last contact.  These attributes are logically
part of `DEVICE` but are very busy, while `DEVICE` is highly connected and mostly static.

The number of readings and the number of actions to keep is TBD, but maybe 10 and 5.

DynamoDB table name: `snappy_history`.  Primary key: `device`.

TODO: There can be additional fields keeping longer-term historical data for the device, but if the
record becomes large these should be split into separate tables.

TODO: `HISTORY` represents a number of tables, one per factor or actuator, and we could consider
splitting them up.  However there are probably few factors per device and the `HISTORY` entry is
always updated for all factors at the same time.

```
HISTORY
    device: <string, device-id>
    last_contact: <positive integer, server timestamp>
    readings: [{factor: <string, factor-id>,
                  last: [{time: <positive integer, server timestamp>,
                          value: <number, value of reading> },
                         ...]},
                 ...]
    actions: [{factor: <string, factor-id>,
                 last: [{time: <positive integer, server timestamp>,
                         reading: <number, value sent to device>,
                         ideal: <number, value sent to device>},
                        ...],
                ...]
    ... additional history fields
```

## Aggregate

TODO: This is a table keyed on location (and maybe other things) that holds aggregate readings
per locations.

## Class

There is one entry in `CLASS` for each device class.  These are are mostly informational and for
optimizing communication, further use TBD.

DynamoDB table name: `snappy_class`.  Primary key: `class`.

```
CLASS
    class: <string, class-id>
    description: <string, human-readable text>
```

Examples:
```
    class: "snappysense", description: "SnappySense"
    class: "rpi2b+", description: "Raspberry Pi 2 Model B+"
    class: "rpi1b+", description: "Raspberry Pi 1 Model B+"
    class: "humidifierxyz", description: "Bosch Humidifier XYZ"
```

## Factor

There is one entry in `FACTOR` for each type of measurement factor known to the sensor fleet and the
code.  When a new `FACTOR` is added it's because we want to add a new device with a new kind of
sensor, or want to add a new sensor to an existing device.

DynamoDB table name: `snappy_factor`.  Primary key: `factor`.

```
FACTOR
    factor: <string, factor-id>
    description: <string, human-readable text>
```
Example:
```
    factor: "temperature",
    description: "Temperature in degrees Celsius"
```

## Ideal

Ideal functions are embedded in the server code and the `IDEAL` table can be updated only when the
code is updated at the same time.

DynamoDB table name: `snappy_idealfn`.  Primary key: `ideal`.

```
IDEAL
    ideal: <string, ideal-id>
    arity: <nonnegative integer, number of arguments to expect>
    description: <string, human-readable text>
```

Example:
```
    ideal: "constant",
    arity: 1,
    description: "Returns its argument"
```

Example:
```
    ideal: "work_temperature",
    arity: 2,
    description: "Adjust temperature according to a workday.  Arguments are temperatures for working hours and off hours"
```