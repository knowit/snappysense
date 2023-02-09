# Factory provisioning

## Individual factory provisioning

In "individual factory provisioning mode" the SnappySense device is brought up in "access point
mode" (AP mode) as described in UX.md, offering a WiFi access point with a name advertised on its
screen (the AP name is generated randomly if none has been cconfigured).  The screen will also show
an IP address.

### Factory config query

To query the device configuration, run

```
   curl http://192.168.a.b/show
```

Passwords are represented by their first letter and certificates by their first line.

### Factory config update

To configure the device, run

```
   curl --data-binary @snp_x_y_no_z.cfg http://192.168.a.b/config
```

where `snp_x_y_no_z.cfg` is the name of the configuration file for the device whose name is
`snp_x_y_no_z`, see `aws-iot-backend/README.md`, and `192.168.a.b` is the IP address printed on the
device screen.

Then reset the device.  To check that the device has been configured properly one can connect to it
with a terminal from VS Code (if the software is set up with the serial command server) and use the
`config` command to print the current configuration.

The syntax of the config file is as follows.  (An example configuration file is also in
`firmware-arduino/src/configuration_template.txt`, and it may be useful to read
`aws-iot-backend/README.md`.)

The config file has a line-oriented syntax.  Blank lines are ignored and generally whitespace is
insignificant except within quoted values and in the payloads for `cert`.  Comment lines start with
`#` (with no leading whitespace) and go until EOL.  Comments should appear only on lines by
themselves.

Annotated grammar:
```
    Program ::= Statements End
      -- A program is a sequence of statements followed by an End line

    End ::= "end" EOL

    Statement ::= Clear | Version | Save | Set | Cert

    Clear ::= "clear" EOL
      -- This resets all variables to their defaults, most of them empty.

    Version ::= "version" Major-dot-Minor-dot-Bugfix EOL
      -- This optional statement states the version of the firmware that the
      -- configuration file was written for.

    Save ::= "save" EOL
      -- This saves all variables to nonvolatile storage.  Usually this comes
	  -- right before the End line; it can be omitted for testing purposes.

    Set ::= "set" Variable Value EOL
      -- This assigns Value to Variable
      -- Value is a string of non-whitespace characters, or a quoted string

    Cert ::= "cert" Cert-name EOL Text-lines
      -- This defines a multi-line text variable
      -- The first payload line must start with the usual "-----BEGIN ..."
	  -- The last payload line must end with with the usual "-----END ..."
	  -- No blank lines or comments may appear after the "cert" line until
	  -- after the last payload line.
```

## Re-provisioning

It's important not to proliferate device IDs and certs too much, so save the config file and other
key files for each device for use during future re-provisioning, should it become necessary.

## Bulk provisioning (sketch; not implemented, but not hard)

Note that the above method using `curl` to post text files is somewhat easily scriptable.  The
script would be given the name of the device and its network and IP address, would contact AWS to
generate the necessary files, would create a configuration, and would upload it.  The thing that's
hardest to script is probably the connection to the device's AP.
