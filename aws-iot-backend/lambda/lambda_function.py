# -*- fill-column: 100 -*-
#
# Overall event handling for SnappySense

import snappy_startup
import snappy_reading
import snappy_mqtt
import snappy_data

mock_mqtt = False
mocked_outgoing = []

def snappysense_event(event, context):
    global mock_mqtt, mocked_outgoing
    if mock_mqtt:
        mocked_outgoing = []
    db = snappy_data.connect()
    if event["message_type"] == "startup":
        responses = snappy_startup.handle_startup_event(db, event, context)
    else:
        responses = snappy_reading.handle_reading_event(db, event, context)
    if mock_mqtt:
        mocked_outgoing = responses
    else:
        for resp in responses:
            snappy_mqtt.publish(resp[0], resp[1], resp[2])
