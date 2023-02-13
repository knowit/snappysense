-*- fill-column: 100 -*-

# AWS IoT backend for SnappySense

## High-level SnappySense network architecture

Each device in the SnappySense network can optionally report its readings to AWS IoT, where they
will be ingested and stored in a database for display and analysis.  Also optionally, AWS IoT can
transmit data back to the device to control it in various ways.

(Devices in the network can be SnappySense devices of various kinds but also sensors connected to
other types of devices; for simplicity I will only consider the standard SnappySense devices in the
following.)

Currently the only communication protocol between a device and AWS IoT is MQTT, a pub/sub protocol.
The device publishes readings and subscribes to control messages.  The server subscribes to readings
and publishes control messages.  The protocol is defined in MQTT-PROTOCOL.md.

Each device must be configured so that it can connect to AWS IoT: It must be provisioned with an AWS
"Thing" identity and the necessary certificates for that Thing.  A section below describes how to
create a new Thing, obtain its identity documents, and install them on SnappySense devices.

SnappySense devices communicate by WiFi and need to be configured for the WiFi available where they
are; they also need to be given a location name appropriate for the location.  This end-user process
is described in [../UX.md](../UX.md).

On the backend, data are stored in a DynamoDB database.  (More here about how to access that from a
convenient UI, TBD.  This UI must allow the registration of things, locations, factors, and so on.
It must allow alerts to be viewed and stats to be shown.)


## Creating a Thing and obtaining its identity documents

- Creating a role for the thing
- Naming the thing, it is usually `snp_x_y_no_z` for version x.y devices, s/n z within that version.
- Creating the thing
- Creating a directory `snp_x_y_no_z` to hold secrets
- Downloading the information into that directory


### Creating certs for an IoT device

(NOT FINISHED)

(This is suitable for a small fleet of devices.  There are other methods.)

Open the AWS Management console as **Dataplattform production** if the cert is going to be
long-lived and not just for testing / prototyping.

Go to AWS IoT.  Under "Manage > Things", ask to create a single thing.

The name of a SnappySense device is `snp_x_y_no_z` where `x.y` is the device type version (1.0 or
1.1 at the time of writing; it's printed on the front of the circuit board) and `z` is the serial
number within that type, usually handwritten on the underside of the processor board.

Make sure **Thing Type** is `SnappySense`.

The thing should have no device shadow, at this point.

On the next screen, auto-generate certificates.

You need to have created an IoT policy for these devices (see separate section).  On the next
screen, pick that policy and then "Create Thing".

You'll be presented with a download screen.  In a new directory, called `snp_x_y_no_z` as above,
store all five files.

### Klient

(NOT FINISHED)

Hver klient skal ha et sertifikat med en AWS IoT Core Policy som tillater maksimalt:

* `iot:Connect`
* `iot:Subscribe` til `snappy/control/+`
* `iot:Subscribe` til `snappy/command/+`
* `iot:Publish` til `snappy/startup/+/+`
* `iot:Publish` til `snappy/reading/+/+`.

TODO: Aller best ville være om vi kunne kvitte oss med `+` og pub/sub bare kan være med devicets egen klasse
og ID, men det er foreløpig uklart om dette lar seg gjøre i en stor flåte av devices.  Men i tillegg til
sikkerhet er det en annen fordel med en mer restriktiv policy: det blir mye mindre datatrafikk i en stor
flåte av devices.

### Server

(NOT FINISHED)

En server / lambda skal ha lov å lytte på og publisere til alt, antakelig.

The policy that allows an IoT route to route data to a lambda is created in IAM.  I have one that
looks like this, called `my-iot-policy`:

```
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "lambda:*",
            "Resource": "*"
        }
    ]
}
```


## Factory provisioning a SnappySense device

Start by making a copy of `firmware-arduino/src/configuration_template.txt` into a new file in the
directory `snp_x_y_no_z`.  I find it useful to call this file `snp_x_y_no_z.cfg`.  (Or copy a file
you already have, but be careful to update the AWS ID and certs in the file.)  The format of this
file is described in detail in [../CONFIG.md](../CONFIG.md).

_This file will contain secrets and must not be checked into github._

Add the necessary information to the new file as described in the comments.  The `aws-iot-id` shall
be the name of the device, `snp_x_y_no_z`, as chosen above.  The device certificate, private key,
and root cert must be the ones downloaded.

The WiFi information in the config file is optional (all three networks) and can be configured by
the end-user.  However it may be useful to configure one network here to allow for factory testing.

The location information is also optional and can be configured by the end-user.

Once the file is complete, save it.

Upload the configuration file to the device as described in [../CONFIG.md](../CONFIG.md), then view
the installed config to check it, also described in that document.


## Saving the secrets for later (IMPORTANT!)

The configuration file contains secrets and will need to be stored securely.  Also, some of the
secrets can't be downloaded from AWS more than the once.  We want to keep both the config file and
the certificate files safely for later use.  Follow this procedure:

In the parent directory of `snp_x_y_no_z`, create a zip file containing the configuration:
```
  zip -r snp_x_y_no_z.zip snp_x_y_no_z/
```

If the cert was created under the Dataplattform **production** account then upload the zip file to
the Dataplattform 1Password account as a new "Document" type with the title `AWS IoT snp_x_y_no_z`.
If there's anything to note, add it to the Notes field.

If the cert was created under your own Dataplattform sandbox account then upload the file instead to
your own 1Password account.
