# AWS IoT backend for SnappySense

(Why is this file not the same file as the DESIGN file?)

The SnappySense device uploads measurement data to an MQTT broker periodically, and can also receive
commands or adjustments from the broker.

### Status pr 2022-12-14

- Det er et greit design på plass, se `DESIGN.md`, `DATA-MODEL.md` og `MQTT-PROTOCOL.md` for
  henholdsvis overordnet design, beskrivelse av data i DynamoDB, og beskrivelse av protokollen
  via MQTT for kommunikasjon mellom device og server.

- Klient-koden er operasjonell (med simulert sensor pr nå), denne kjører på Linux og bruker lite
  ressurser, se `mqtt-client/README.md`.  Denne vil være helt OK for sensorer og aktuatorer som
  kjører på embedded Linux, i hvert fall.

- Server-koden for klient-kommunikasjon (mot AWS Lambda med DynamoDB) er operasjonell, mer eller
  mindre (testet lokalt), se `lambda/README.md`.  Den håndterer mottak av sensorlesinger og sender
  meldinger tilbake til aktuatorer.

- Server-koden for REST-API for web server er under utarbeidelse.


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
