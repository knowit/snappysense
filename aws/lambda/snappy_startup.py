# -*- fill-column: 100 -*-
#
# Respond to a SnappySense "startup" MQTT package by recording the time in the history for the
# device and computing and sending relevant configuration commands for the device.
#
# See ../MQTT-PROTOCOL.md for a description of the messages.
# See ../DATA-MODEL.md for a description of the databases.

import snappy_data
import math
import time

# Startup event
#
# Input fields
#   message_type  - string, value "startup"
#   device        - string, device ID from snappy_device table
#   class         - string, class ID from snappy_class table
#   sent          - integer, device timestamp (seconds since epoch, or junk)
#   interval      - integer

# Returns array of mqtt responses: [[topic, payload, qos], ...]

def handle_startup_event(db, event, context):
    device = event["device"]
    device_class = event["class"]
    sent = event["sent"]
    reported_interval = event["interval"]
    received = math.floor(time.time())

    device_entry = snappy_data.get_device(db, device)
    if not device_entry:
        # Device does not exist, this is an error.  TODO: Logging?
        return []

    # Update the device entry with time of last contact.  We assume server time is monotonic...

    snappy_data.device_set_last_contact(device_entry, received)
    snappy_data.write_device_entry(db, device_entry)

    # Configure the device if necessary.

    payload = {}
    if not snappy_data.device_enabled(device_entry):
        payload["enabled"] = 0

    set_interval = snappy_data.device_interval(device_entry)
    if reported_interval != 0 and reported_interval != set_interval:
        payload["interval"] = set_interval
    
    # Send the configuration with QoS1 to ensure that the message is received.  The device needs to
    # be prepared to receive several config commands after it connects, as a retained or queued
    # command may be delivered first followed by a new command.

    if payload:
        # Version 1.0.0 has "enabled" and "interval".  For rules about the semver of this
        # JSON package, see firmware-arduino/src/mqtt.cpp, in mqtt_handle_message()
        payload["version"] = "1.0.0"
        return [[f"snappy/control/{device}", payload, 1]]
    return []
