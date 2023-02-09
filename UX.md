# User experience, configuration, etc

(This is for the Arduino-based firmware only, for the time being.  The IDF-based firmware has its
own documentation.)

## Day-to-day use

The SnappySense device has two modes, selectable by pressing the front button at run-time:
_slideshow_ mode and _monitoring_ mode.

In slideshow mode, SnappySense is on continually, reads the environment frequently, and displays a
repeating slideshow of the readings on its little screen.  In this mode, SnappySense needs to be on
an external power supply.

In monitoring mode, SnappySense is usually in a low-power state, but wakes up from time to time
(usually about once an hour) to read the environment and upload the readings.  The display is
briefly on when the readings are made but no data are displayed, only a splash screen.  In this
mode, SnappySense can run on battery power for some time.

If SnappySense is appropriately configured, the readings are uploaded occasionally to AWS IoT for
processing and storage.  If uploading is not possible because no network is reachable, then readings
are gradually discarded as memory fills up.

## Access point mode

The SnappySense device can be brought up in "access point mode" (AP mode), allowing it to be
configured and for its configuration to be queried.

To enter AP mode, press and hold the button on the front of SnappySense (labeled WAKE on 1.0.0 and
BTN1 on 1.1.0) and press and release the RESET button on the back.  When SnappySense enters
provisioning mode, a message on its little screen will show a network name and an IP address on the
form `192.168.x.y`.  If the device has not been given an access point name, it will generate a
random one.

### End-user configuration

In AP mode, the user can perform _end-user configuration_ by connecting to the named WiFi network
(ignore any errors you may see about the network not providing internet access).  Then in a browser
on the connected device, open a page at `http://192.168.x.y`.  (Be sure not to use `https` at this
time.)  You will see a form with configuration options.  Change what you want to change, then press
the SUBMIT button to store the options on SnappySense.

After configuration, press the RESET button on the back of SnappySense to restart it with the new
settings.

The configuration values are in two groups: the names and passwords for any networks it can connect
to for uploading data (up to three networks), and the name of the location the device is in.  This
name is not quite arbitrary, it should be obtained from somebody with access to the monitoring data
stored on AWS, as the name is used to help identify the data and should be unique and meaningful.

### Factory provisioning

In AP mode, the device similarly responds to factory configuration and query commands.  These are
more complicated, not usefully performed after initial setup, and described in CONFIG.md.
