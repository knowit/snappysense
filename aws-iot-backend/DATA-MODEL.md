-*- fill-column: 100 -*-

# SnappySense Data Model

This is the data model definition for the SnappySense back-end based on AWS Lambda.  For notes on
the overall design, see DESIGN.md in this directory.

This back-end maintains or uses a number of AWS DynamoDB tables: LOCATION (`snappy_location`),
DEVICE (`snappy_device`), CLASS (`snappy_class`), FACTOR (`snappy_factor`), and HISTORY
(`snappy_history`).  The first four are mostly static, while HISTORY is updated in response to data
received from the devices.

Right now this is a NoSQL model, and the data stored in HISTORY (and to some extent in LOCATION) are
not uniform, but vary with the device that reported data (or the nature of the location).

Technically speaking the data model is defined not here, but jointly by two programs: the AWS Lambda
code in `lambda/`, and the database command-line interface in `dbop/`.  Those programs MUST be in
agreement, and they jointly also describe the JSON layout expected by the databases.  (For example,
whether something is an "L" list containing string values or an "SS" string set, and whether
something is an "N" number or an "S" string that happens always to have a numeric value.)

Whatever is in the present document is therefore only human-level documentation (though it should
agree with the programs, of course).

(Previously there were two other tables, IDEAL and AGGREGATE.  Those have been removed, and prose
referring to them should be ignored.  Also, this data model has been simplified significantly from
some earlier drafts.)

## LOCATION

An entry in `LOCATION` represents a location where we can place sensors and actuators.

DynamoDB table name: `snappy_location`.  Primary key: `location`.

```
LOCATION
    location: <string: location-id>
    description: <string: human-readable, fairly specific>
```

Example:
```
    name: "lars-t-hansen-hjemmekontor"
    description: "Gjemmekontoret til Lars T, dypt inne i Lillomarka"
```

TODO: This could have geolocation, modulo privacy issues.

TODO: This could have time zone or really, the political time domain, so that computing eg "work
time" vs "non-work time" is possible.

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

TODO: It is a bug that a device name cannot match any device class name or the string `all`; the bug is
really in the control message protocol and should be fixed there.

TODO: It's possible that the `reading_interval` on a device should be per environmental factor and
not for the device as a whole, and that the default `reading_interval` for a factor should be stored
in `FACTOR`.


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

TODO: It is a bug that a class name cannot match any device name or the string `all`; the bug is
really in the control message protocol and should be fixed there.

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

## HISTORY

There is one entry in `HISTORY` for each device.  The entry holds the "last" readings from the device
and information about the time of last contact.  These attributes are logically part of `DEVICE` but
are very busy, while `DEVICE` is highly connected and mostly static.

DynamoDB table name: `snappy_history`.  Primary key: `device`.

```
HISTORY
    device: <string, device-id>
    last_seen: <string, server timestamp>
    readings: [{factor: <string, factor-id>,
                time: <string, client timestamp>,
                value: <number, value of reading>}, ...]
```

The readings are unsorted (for now) and the list is flat to keep the data model simple (as opposed
to organized under `factor`).  When the backend wants to expire old entries it needs to do the work
to sort and organize the values at that time.  A sensible strategy might be to do the work whenever
the first reading arrives after midnight, aggregating the previous day's entries in some TBD
fashion.

TODO: An alternative and even simpler model is that the primary key is synthetic and that `HISTORY`
functions mostly as a log.  Readings are added blindly to the log.  Once a day, or every so often,
the log is consolidated into aggregate data and cleared; this can happen concurrently with new
entries arriving.  In this model, the `device` and `last_seen` fields are moved into the record of
readings.

Are there DynamoDB synthesized keys or something like that?  Note, there is really not a "primary"
key in DynamoDB, only "partition" keys that need not be unique.  We have made them unique.  But they
don't have to be.  The "partition" key could be a random number, and that would work fine.  For
small data volumes it could even be the timestamp, which is not guaranteed to be unique.  And for an
append-only log this is just fine - we'll never really search it.  When we need to remove something,
though, we'll possibly need to be careful...  But this is no worse than giving each record a field
that is initially blank and is set to "X" in records that are to be deleted?  Indeed this field
could be the timestamp field....

DynamoDB uses "primary key" about the combination of partition key and sort key.  So perhaps these
really need to be a unique pair.  But that's not a hard requirement to contend with.  The expiration
process can just set unique integer values in a sort key and use that.

This leads to this design:

DynamoDB table name: `snappy_history`.  Primary key: `key`.

```
HISTORY
    key: <string, device + '#' + last_seen + '#' + sequenceno from device>
    device: <string, device-id>
    last_seen: <string, server timestamp>
    time: <string, client timestamp>
	<mangled-factor-name>: <number, value of reading>
	...
```

A mangled factor name has the prefix `F#` added to whatever the device supplied.  This avoids 

Every reading is blindly added to the table using PutItem.

There is an error source here, in that a fast-booting device that makes a reading with sequenceno 0,
reboots, and makes another reading within the same second will only have one of them recorded, BUT I
CAN LIVE WITH THAT.
