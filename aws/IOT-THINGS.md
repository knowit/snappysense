-*- fill-column: 100 -*-

# Managing IoT "things"

This document pertains to how "things" are set up one by one; for a fleet of devices, the
instructions will be different.

There are four major steps.  First, a new Thing is registered with AWS and its identity documents
are obtained.  Second, the Thing is registered with the SnappySense backend.  Third, the device
itself is provisioned with the identity documents.  Finally, the identity documents and device
configuration are stored safely for later reuse.

## Creating a Thing and obtaining its identity documents

Every device ("Thing") needs a unique _Thing Name_.  The format for a SnappySense Thing name shall
be `snp_x_y_no_z` for devices with hardware version `x.y` and serial number `z` within that version,
ie, `snp_1_2_no_37` has hardware v1.2 and serial number 37.  Non-SnappySense Things should use their
own, compact, class designators (eg `rpi` would make sense for Raspberry Pis).

Every Thing also has a _Thing Type_, this aids device management.  For SnappySense Things, the Thing
type is `SnappySense`.  Non-SnappySense Things should have Thing Types that are descriptive (eg
`RaspberryPi`); stick to letters, digits, `-` and `.`, or you'll be sorry.  (And if you plan to use
RabbitMQ as your message broker, avoid `.` as well.)

In addition to being created in AWS IoT, the Thing must be registered in our backend databases; see
the "Registering a new Thing with the SnappySense backend" section below.

Every Thing is associated with a number of files containing secrets specific to the Thing:
certificates from AWS, and a configuration file containing these certificates along with WiFi
passwords and similar.  As a matter of hygiene, these files should all be stored together in a
directory that has the same name as the Thing.  Also see the sections below on "Factory
provisioning" and "Saving the secrets for later".

To create a new Thing in AWS IoT, follow the steps outlined below.  These steps will be the same
whether you are using the **Dataplattform production console** or your own **Sandbox console**.  If
the Thing's identity will be long-lived then you should _not_ create it in your sandbox.

### Creating a security policy

You need to create a security policy for SnappySense devices if it does not already exist; you need
to do this only once.  It should be called `SnappySensePolicy`.  The JSON for the policy is in the
file `thing_policy.json` in the current directory.

Go to AWS > Management console > IoT Core > Security > Policies.

To create the policy, click `Create Policy`, then select the JSON option and paste in the JSON from
`thing_policy.json`, then click to confirm creation.

All things in the SnappySense network, whether SnappySense things or not, can use the same policy.

TODO: How to do this with the AWS CLI or something like `dbop`, including checking whether it needs
to be done.

### Creating the Thing and obtaining its documents
  
Create a new directory somewhere on your PC that will hold the secrets for the new Thing.  This
directory should have the same name as the Thing you are creating.

Go to AWS > Management console > IoT Core > All devices > Things.  Then:

* Click on "Create things", select "Create single thing", click "Next"
* The Thing Name has the format `snp_x_y_no_z` as explained in more detail above
* The Thing Type is `SnappySense`.  (If you have to create the thing type first, then the only thing
  we care about is the name; the description can be blank or something like "SnappySense devices".)
* The Thing has no device shadow.
* Click "Next"
* Choose to auto-generate certificates; click "Next" again.
* Now you have to select a policy.  Choose `SnappySensePolicy`, created above.  If you did not
  create it already, create it now.
* Click "Create thing".
* You'll be presented with a download screen.  In the directory for the device, created above, store
  all five files.

Congratulations, you have a new Thing.  For information about how to install the documents on the
device and how to keep the secrets safe, see "Factory provisioning" and "Saving the secrets for
later", below.

TODO: How to do this with the AWS CLI or something like `dbop`.

### Registering a new Thing with the SnappySense backend

Before you can do this you must have set up the backend, as described in [SETUP.md](SETUP.md), and you
must have compiled the `dbop` program as described there.

For a normal SnappySense 1.1 device with serial number XX and location YY, run:
```
dbop device add device=snp_1_1_no_XX class=SnappySense location=YY factors=temperature,humidity,uv,light,pressure,airsensor,airquality,tvoc,co2,motion,noise
```

To inspect devices already in the table, run:
```
dbop device list
```

If you know how, you can also manipulate the databases through the AWS DynamoDB dashboard.

### Some notes about the security policy (for the specially interested only)

The current (February 2023) security policy in `thing_policy.json` very lax.  It would be better to
restrict it to these actions only:

* `iot:Connect`
* `iot:Subscribe` for `snappy/control/+`
* `iot:Subscribe` for `snappy/command/+`
* `iot:Publish` for `snappy/startup/+/+`
* `iot:Publish` for `snappy/observation/+/+`.

The restrictions would be more secure, as it would make it impossible for Things to subscribe to
non-SnappySense messages flowing through the AWS MQTT broker.

Even better would be if a Thing would only be allowed to subscribe to those messages that are sent
to itself: this would require a separate policy per Thing however, and might not be practical in a
large fleet of devices.

## Factory provisioning a SnappySense device

Start by making a copy of `firmware-arduino/src/configuration_template.txt` into a new file in the
directory `snp_x_y_no_z`.  I find it useful to call this file `snp_x_y_no_z.cfg`.  (Or if you like,
copy a file you already have, but be careful to update the AWS ID and certs in the file.)  The
format of this file is described in detail in [../CONFIG.md](../CONFIG.md).

_This file will contain secrets and must not be checked into github._

Add the necessary information to the new file as described in the comments.  The `mqtt-id` shall
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
the certificate files in a safe place for later use.  Follow this procedure:

In the parent directory of `snp_x_y_no_z`, create a zip file containing the configuration:
```
  zip -r snp_x_y_no_z.zip snp_x_y_no_z/
```

Now **upload that zip file to 1Password** as a new "Document" type with the title `AWS IoT snp_x_y_no_z`.
If there's anything to note, add it to the Notes field in 1Password.

If the cert was created under the Dataplattform **production** account then use the Dataplattform
1Password account; if you used your AWS sandbox account, then upload the file to your own 1Password
account.
