-*- fill-column: 100 -*-

# Back-end and front-end support for SnappySense

The devices in the SnappySense network (whether they be actual SnappySense devices or something else
with a sensor) can report sensor readings to AWS IoT, which stores them in a database.

Web browsers can connect to AWS to receive a web application that allows those data to be viewed and
modified.

A single AWS Lambda function is the nexus for this logic.  When contacted from AWS IoT over MQTT, it
ingests observations and generates control messages that are shipped by MQTT back to the devices.
When contacted from a browser over HTTP, it serves static assets and query responses.

Each device must be configured so that it can connect to AWS IoT: It must be provisioned with an AWS
"Thing" identity and the necessary certificates for that Thing.  SnappySense devices communicate by
WiFi and also need to be configured for the WiFi available at their location.

## Documentation

At the moment, a fair amount of manual work goes into setting up the various parts of the system.  The
required work is described in several documents:

* [SETUP.md](SETUP.md) describes how to set up the AWS IoT / Lambda / DynamoDB back-end and test
  that it works; this is required for anything else to make sense.  You do this once per sandbox or
  production environment.
* [IOT-THINGS.md](IOT-THINGS.md) describes how to register new "Things" with AWS IoT and with the
  SnappySense back-end and install identity documents on each Thing.  There's some initial setup
  work and then a small amount of work per new Thing.
* [../firmware-arduino/MANUAL.md](../firmware-arduino/MANUAL.md) describes how each SnappySense
  Thing is configured with WiFi access.
* [FRONTEND.md](FRONTEND.md) describes how to use the front end, which allows the ingested data to
  be viewed (though not yet modified)

Current technical documents about data formats and protocols:

* [DATA-MODEL.md](DATA-MODEL.md) describes the data model of the back-end databases.
* [../MQTT-PROTOCOL.md](../MQTT-PROTOCOL.md) describes the details of the communication protocols used to 
  communicate between the devices and AWS IoT (or any other MQTT broker).

Code is mostly in subdirectories:

* [lambda/](lambda/) has the AWS Lambda function that implements the back-end.
* [dbop/](dbop/) has a utility command line program that talks directly to the databases (bypassing the 
  lambda) and allows tables to be created, deleted, and inspected.
* [test-code/](test-code/) has some simple test programs that can be used during the setup phase,
  see [test-code/README.md](test-code/README.md).
