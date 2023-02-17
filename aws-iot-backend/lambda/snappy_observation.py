# -*- fill-column: 100 -*-
#
# Respond to a SnappySense "observation" message by recording the observation in the OBSERVATION table
# and computing and sending relevant actuator commands for the location of the device.
#
# See ../MQTT-PROTOCOL.md for a description of the messages.
# See ../DATA-MODEL.md for a description of the databases.

import snappy_data
import time
import math
import random

# Input fields
#   message_type - string, value "observation"
#   device       - string
#   class        - string
#   sent         - integer, seconds since epoch device time
#   sequenceno   - integer
#   location     - string, may be absent
#   F#<factor>   - number
#   F#<factor>   - number
#   ...

# Returns array of mqtt responses: [[topic, payload, qos], ...]

def handle_observation_event(db, event, context):
    device = event["device"]
    device_class = event["class"]
    sent = event["sent"]
    sequenceno = event["sequenceno"]
    # TODO: Probably if location is absent we should get it from the device record?
    location = event["location"] if "location" in event else ""
    factors = {}
    for k in event:
        if k.startswith("F#"):
            factors[k] = event[k]
    received = math.floor(time.time())
    salt = random.randint(1, 100000)
    key = device + "#" + str(received) + "#" + str(sequenceno) + "#" + str(salt)

    snappy_data.put_observation(db, key, device, location, sequenceno, received, sent, factors)

    # Here we could check if the device has become reconfigured on the back-end (eg become disabled)
    # and if so send control messages.  For now, do nothing - it's more sensible for the activity
    # that disables the device to trigger the control message directly.

    return []
