MQTT upload, ideally directly to AWS.  This means that the device must somehow
hold certificate, private key, root CA, device name, maybe endpoint URL, and misc
other things - quite a lot of data to store in a small EEPROM!  So it'll
be interesting to figure that out.

Decouple time service from data upload - the time service could use AWS lambda
for example.

The TVOC and CO2 sensors were initially working fine but have since stopped
working.  They will function for the first couple of readings but then
start responding with zero values.  I don't know why this is - nothing
substantial has changed very much to break the readings.

Should incorporate ideas from Gunnar re how to put the device to sleep.

Parameters should be stored in some kind of parameter area on the device,
flash or EEPROM - not hardcoded.

Sensors need to be calibrated - they tend to be off by quite a bit.
Temperature readings are usually 4-6 degrees above that reported by other
thermometers in the same location, for example.

The altitude measurement is just junk.  Looking at the code in the DFRobot
library, it reads barometric pressure and applies an undocumented formula
to it.  The results are bizarre and altitude readings are off by several
hundred meters and fluctuate hugely with the pressure.

The command parser in SerialCommands.cpp can maybe be repurposed for
some of the web server stuff.

Various FIXME and TODO comments in the code.
