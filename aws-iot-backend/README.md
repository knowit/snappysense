# AWS IoT backend for SnappySense

The SnappySense device uploads measurement data to an MQTT broker periodically, and can also receive
commands or adjusments from the broker.

## Creating policies for IoT devices

To be written.  We need to create a policy just once and this can be done manually.

## Creating certs for an IoT device

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

## Factory provisioning a SnappySense device

Start by making a copy of `firmware-arduino/src/configuration_template.txt` into a new file in the
directory `snp_x_y_no_z`.  I find it useful to call this file `snp_x_y_no_z.cfg`.  (Or copy a file
you already have, but be careful to update the AWS ID and certs in the file.)

_This file will contain secrets and must not be checked into github._

Add the necessary information to this file as described in the comments.  The `aws-iot-id` shall be
the name of the device, `snp_x_y_no_z`, as chosen above.  The device certificate, private key, and
root cert must be the ones downloaded.

The WiFi information is optional (all three networks) and can be configured by the end-user.
However it may be useful to configure one network here to allow for factory testing.

The location information is also optional and can be configured by the end-user.

Once the file is complete, save it.

Connect the device to USB with a terminal emulator running.  Press and hold the button on the front
of the device (labeled WAKE or BTN1, depending on version) and then press and release the reset
button on the CPU.  After about 2 seconds the device will come up in configuration mode, the screen
will read "Configuration mode".

In
the terminal emulator, type `view` to check that the device is operational (there should be
meaningful output).  Now copy the config file to the clipboard and paste it in in the terminal
emulator.

## Saving the configuration for later (IMPORTANT!)

The configuration contains secrets and will be stored securely.  We want to keep both the config
file and the certificate files.

In the parent directory of `snp_x_y_no_z`, create a zip file containing the configuration:
```
  zip -r snp_x_y_no_z.zip snp_x_y_no_z/
```

(Tentative) Upload the zip file to the Dataplattform 1Password account as a new "Document" type with
the title "AWS IoT snp_x_y_no_z", if the cert was created under **production**.  If there's anything
to note, add it to the Notes field.
