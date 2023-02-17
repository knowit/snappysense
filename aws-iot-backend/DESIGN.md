-*- fill-column: 100 -*-

# SnappySense third-generation prototype, 2022/2023

This document is about the technical architecture of the backend.  It is still rough.

For general background and high-level design, see [`README.md`](README.md) and
[`../BACKGROUND.md`](../BACKGROUND.md).  The following assumes that knowledge.

## Overall back-end architecture

The SnappySense devices perform measurements and hold them for periodic upload by MQTT (see
[`MQTT-PROTOCOL.md`](MQTT-PROTOCOL.md)).  When an MQTT message arrives at AWS IoT, matching MQTT
topics are routed to AWS Lambda.  The Lambda functions store the data in DynamoDB databases (see
[`DATA-MODEL.md`](DATA-MODEL.md)).

There must eventually exist a dashboard that allows new devices and locations and other metadata to
be defined in the databases, and allow the measurements stored in the databases to be viewed.  In
the mean time, there is a command line program that makes database access bearable; see below.

By way of overview:

- The database has mostly-static tables DEVICE (for device IDs and capabilities), CLASS (for device
  classes), LOCATION (for known locations where devices can reside), and FACTOR (for the types of
  things that can be measured).

- The database has a dynamic tables OBSERVATION (for the most recent sensor readings for each
  device) and will in the future have a table for aggregate data.
  
The tables are described in detail [`DATA-MODEL.md`](DATA-MODEL.md).

When a new device is registered it is always added to DEVICE.  If it belongs to a new device class,
then CLASS must be updated.  If it has a new type of sensor, then FACTOR must be updated.  If it is
going to be placed in a not previously known location, then LOCATION must be updated.

When a "startup" message arrives from a device, a control message may be generated to (re)configure
the device if the device's reported configuration differs from what's desired.  (The desired
configuration is stored in the DEVICE table.)

When a "reading" arrives from a device, a new OBSERVATION entry is created to add the new readings.
Occasionally, the OBSERVATION table is going to have to be culled and data moved into some place
for aggregated data; this is TBD.

## Is Lambda and DynamoDB what we ought to be using?

Possibly not, but it's a prototype.

Likely, some container running on an EC2 instance, a relational DB and a relational data model make
for a better solution if we ever get to the point where we have some real volume of devices and
readings, and/or we get the UI people to develop proper UIs and backend support for them.

## Dashboards and UIs

TBD - this is a sketch, it's not the current focus.

There must exist various kinds of triggers or alerts: for readings that indicate a very bad
environment; for devices that stop communicating.

There must exist some interface that allows for devices, classes, factors, and locations to
be added.

There must exist some basic interface to inspect all the tables.  At the moment, everything must be
added by the AWS CLI (or maybe from the console, if the console can do what we need).  Obviously
this is not acceptable.

All of this is some basic REST API, maybe the AWS API Gateway or something like that.

## Aggregation

This is for future consideration.

It's currently undefined what "aggregation" means, and no aggregate table is defined in the data
model.  Here are some notes:

- aggregation begs the purpose of gathering the data in the first place
- we want to record "conditions" and monitor them over time and alternatively also provide an
  indicator when "conditions" are bad
- so meaningfully we might consider weekday as the primary key for a datum, as we care more about
  comparing mondays to mondays than january 1 to january 1
- so there may be a table that is indexed by (location, day-of-the-week) and for each entry there is
  a table going back some number of weeks (TBD, but maybe 100?), and in each table element there is
  an object (factor, [{time, reading, ...}, ...])  where the exact fields of the inner structures
  are tbd.  This should be sorted with most recent timestamp first in the inner list.
- there could be a lot of data here if we have one reading per factor per hour, this should be a
  consideration.  In principle, the table could be indexed by a triple, (location, day-of-the-week,
  hour-of-the-day)
- this all makes it easy to report per-location and per-hour and/or per-day-of-the-week.
