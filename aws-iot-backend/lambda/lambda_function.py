# -*- fill-column: 100 -*-
#
# Overall event handling for SnappySense

import snappy_startup
import snappy_reading

def snappysense_event(event, context):
    db = snappy_data.connect()
    if event["message_type"] == "startup":
        responses = handle_startup_event(db, event, context)
    else:
        responses = handle_reading_event(db, event, context)
    for resp in responses:
        snappy_mqtt.publish(resp[0], resp[1], resp[2])
