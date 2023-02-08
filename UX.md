# User experience, configuration, etc

(This is for the Arduino-based firmware only, for the time being.  The IDF-based firmware has its own documentation.)

## Day-to-day use

The SnappySense device has two modes, selectable at end-user configuration time: _slideshow_ mode and _monitoring_ mode.

In slideshow mode, SnappySense is on continually, reads the environment frequently, and displays a repeating slideshow of the readings on its little screen.  In this mode, SnappySense needs to be on an external power supply.

In monitoring mode, SnappySense is usually in a low-power state, but wakes up from time to time (usually about once an hour) to read the environment and upload the readings.  The display is briefly on when the readings are made but no data are displayed, only a splash screen.  In this mode, SnappySense can run on battery power for some time.

If SnappySense is appropriately configured, the readings are uploaded occasionally to AWS IoT for processing and storage.  If uploading is not possible because no network is reachable, then readings are gradually discarded as memory fills up.

## End-user configuration

The SnappySense device can be brought up in end-user configuration mode.  To do this, press and hold the button on the front of SnappySense (labeled WAKE on 1.0.0 and BTN1 on 1.1.0) and press and release the RESET button on the back.  When SnappySense enters configuration mode, a message on its little screen reads "Configuration mode" along with a network name (usually `snappy-<something>-config`) and an IP address on the form `192.168.x.y`.  (If the message reads "No cfg access point" then the factory configuration was faulty.)

On a phone or laptop, connect to the named WiFi network and ignore any errors you may see about the network not providing internet access. Then in a browser on your phone or laptop open a page at `http://192.168.x.y`.  (Be sure not to use `https` at this time.)  You will see a form with configuration options.  Change what you want to change, then press the SUBMIT button to store the options on SnappySense.

After configuration, press the RESET button on the back of SnappySense to restart it with the new settings.

The configuration values are in three groups: the SnappySense mode (slideshow or monitoring), the names and passwords for any networks it can connect to for uploading data (up to three networks), and the name of the location the device is in.  This name is not quite arbitrary, it should be obtained from somebody with access to the monitoring data stored on AWS, as the name is used to help identify the data and should be unique and meaningful.

## Individual factory provisioning

In "individual factory provisioning mode" the SnappySense device is brought up in configuration mode as above, but it is also connected to a terminal over its USB connection.  In this mode, the device will accept commands and configuration data via the terminal.  The way this will normally work is that a configuration file is prepared containing the configuration data for the device, and then simply pasted into the terminal.

An example configuration file is in src/configuration_template.txt.

## Bulk provisioning (sketch; not implemented, but not hard)

If we have a lot of devices, then the individual factory provisioning is going to be tedious;  instead we want something that is scriptable.  Here is a sketch for how that might work:

- The device knows the name of a provisioning network, this is compiled into the code, say.
- There is an nvram setting, `provisioned`, which is set once factory provisioning has been performed.
- When the device boots up and that setting is not set, it goes into bulk provisioning mode: it connects to the known network and retrieves a blob of data containing all the usual factory settings.  During this protocol the device tells the server its device class, which is also compiled into the code, but everything else is gotten from the server.
- Once the data have been stored on the device the `provisioned` flag is set and the device is ready.
- On the server side, the process can be scripted: when a new device connects it can, if necessary, generate the appropriate data for the device on the fly: a new device name, new certificates and ID values from AWS IoT and so on.
- Clearly it's possible for a device to be re-provisioned by flipping the `provisioned` flag to zero; if this happens, it would be sensible for the device to notify the server of its existing name during the protocol, so that we don't pointlessly proliferate device names and certs.  But this is just a detail.
