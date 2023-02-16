-*- fill-column: 100 -*-

# SnappySense Data Model

This is the data model definition for the SnappySense back-end based on AWS Lambda.  For notes on
the overall design, see DESIGN.md in this directory.

This back-end maintains or uses a number of AWS DynamoDB tables: LOCATION (`snappy_location`),
DEVICE (`snappy_device`), CLASS (`snappy_class`), FACTOR (`snappy_factor`), and OBSERVATIONS
(`snappy_observations`).  The first four are mostly static, while OBSERVATIONS is updated in
response to data received from the devices.

Right now this is a NoSQL model, and the data stored in OBSERVATIONS (and to some extent in
LOCATION) are not uniform, but vary with the device that reported data (or the nature of the
location).

Technically speaking the data model is defined not here, but jointly by two programs: the AWS Lambda
code in `lambda/`, and the database command-line interface in `dbop/`.  Those programs MUST be in
agreement, and they jointly also describe the JSON layout expected by the databases.  (For example,
whether something is an "L" list containing string values or an "SS" string set, and whether
something is an "N" number or an "S" string that happens always to have a numeric value.)

Whatever is in the present document is therefore only human-level documentation (though it should
agree with the programs, of course).

## LOCATION

An entry in `LOCATION` represents a location where we can place sensors and actuators.

DynamoDB table name: `snappy_location`.  Primary key: `location`.

```
LOCATION
    location: <string: location-id>
    description: <string: human-readable, fairly specific>
	devices: <list of string: device IDs at this location>
	timezone: <string, time location eg "Europe/Berlin", see iana.org/time-zones or timezonedb.com>
```

Example:
```
    name: "lars-t-hansen-hjemmekontor"
    description: "Gjemmekontoret til Lars T, dypt inne i Lillomarka"
```

TODO: This could have geolocation, modulo privacy issues.

## DEVICE

An entry in `DEVICE` represents a single sensor device, possibly containing many distinct sensors
for environmental factors.  The default for `enabled` is 1.  The default for `reading_interval` is
something sensible, on the order of 1 hour.

DynamoDB table name: `snappy_device`.  Primary key: `device`.

```
DEVICE
    device: <string, device-id>
    class: <string, class-id>
    location: <string, location-id>
    enabled: <boolean>
    reading_interval: <positive integer, seconds>
    factors: [<string: factor-id>, ...]
```

The device-id has a format that describes it generally.  For example, the SnappySense devices have
IDs that are on the format `snp_x_y_no_z` where `x.y.0` is the hardware version and `z` is the
serial number of the device within that version.  Device IDs are always AWS IoT device IDs.

Example:
```
    device: "snp_1_1_no_1",
    class: "snappysense",
    location: "lars-t-hansen-hjemmekontor",
    factors: ["temperature","humidity","uv","light","pressure","altitude","airsensor",
	          "airquality","tvoc","co2","motion","noise"],
```


## CLASS

There is one entry in `CLASS` for each device class.  These are are mostly informational and for
optimizing communication, further use TBD, but they are used by AWS IoT to manage devices.

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
```

## FACTOR

There is one entry in `FACTOR` for each type of measurement factor known to the sensor fleet and the
code.  When a new `FACTOR` is added it's because we want to add a new device with a new kind of
sensor, or want to add a new sensor type to an existing device.

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

The factors known so far are the ones measured by the SnappySense device.  For uniformity, the
values carried are always numbers, even when bools or strings might make more sense.

* `temperature`: float, degrees celsius
* `humidity`: float, relative in interval 0..100
* `uv`: float, UV index 0..15
* `light`: float, luminous intensity in lux, range unclear
* `altitude`: float, meters relative to sea level
* `pressure`: integer, hecto-Pascals
* `airquality`: integer, 1..5, air quality index
* `airsensor`: integer, 0..3, air sensor status (0..2 mean OK, 3 means broken)
* `tvoc`: integer, range varies, total volatile organic content in ppb
* `co2`: integer, range varies, equivalent co2 content in ppm
* `noise`: integer, range unclear, unit unclear
* `motion`: integer, 0 or 1, whether motion was detected during last measurement window

There can be more factors than these.

Factor names may not start with the character sequence `F#`, as this is used for internal name
mangling purposes.

## OBSERVATIONS

The `OBSERVATIONS` table is a mostly-append-only log of the most recent observations.  When a report
arrives from a device it is lightly processed and added to `OBSERVATIONS` under a unique,
synthesized key.  The intent is that this table is culled every so often, with data being processed
and moved into som aggregate table.

DynamoDB table name: `snappy_observations`.  Primary key: `key`.

```
OBSERVATIONS
    key: <string: device + # + received + # + sequenceno + # + random integer>
    device: <string, device-id>
    received: <string, server timestamp>
	sent: <string, device timestamp>
	sequenceno: <integer, device's observation sequence number since boot>
    F#<factor-name>: <number, observation value for <factor-name>>
    ...
```

There is a minuscule chance that there can be a duplicate key, if the device boots very quickly and
manages to send two observations with the same low sequence number (probably only 0) under the same
time stamp.  In this case, the later observation overwrites the former in the table.  I CAN LIVE
WITH THAT.
