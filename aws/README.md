-*- fill-column: 100 -*-

# Back-end and front-end support for SnappySense

The devices in the SnappySense network (whether they be actual SnappySense devices or something else
with a sensor) can report sensor readings to AWS IoT, which stores them in a database.

Web browsers can connect to AWS to receive a web application that allows those data to be viewed and
modified.

A single AWS Lambda function is the nexus for this logic.  When contacted from AWS IoT over MQTT, it
ingests observations and generates control messages that are shipped by MQTT back to the devices.
When contacted from a browser over HTTP, it serves static assets and query responses.

At the moment, a fair amount of manual work goes into setting up the various parts of the system.  The
required work is described in several documents:

* [SETUP.md](SETUP.md) describes how to set up the AWS IoT / Lambda / DynamoDB back-end and test that 
  it works; this is required for anything else to make sense.
* [IOT-THINGS.md](IOT-THINGS.md) describes how to register "things" with AWS IoT and with the
  SnappySense back-end.
* [FRONTEND.md](FRONTEND.md) describes how to use the front end, which allows the ingested data
  to be viewed (though not yet modified)

Some more or less current design notes and user guides:

* [BACKEND.md](BACKEND.md) has some partly stale design notes for the back-end.
* [TODO.md](TODO.md) is what it looks like: a not always up-to-date to-do list.

Technical documents about data formats and protocols:

* [DATA-MODEL.md](DATA-MODEL.md) describes the data model of the back-end databases.
* [MQTT-PROTOCOL.md](MQTT-PROTOCOL.md) describes the details of the communication protocols used to 
  communicate between the devices and AWS IoT.

Code is mostly in subdirectories:

* The AWS Lambda function that implements the back-end is in `lambda/`.
* A utility command line program that talks directly to the databases (bypassing the lambda) and
  allows tables to be created, deleted, and inspected is in `dbop/`.
* Test programs for the setup phase are in `test-lambdas/`
