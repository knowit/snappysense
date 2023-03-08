# -*- fill-column: 100 -*-
#
# Overall event handling for SnappySense

import snappy_http
import snappy_startup
import snappy_observation
import snappy_mqtt
import snappy_data

# Debugging tools for mqtt

mock_mqtt = False
mocked_outgoing = []

# The event handler dispatches to mqtt or http handling depending on the
# event contents.

def snappysense_event(event, context):
    db = snappy_data.connect()
    if "requestContext" in event and "http" in event["requestContext"]:
        if "queryStringParameters" in event:
            params = event["queryStringParameters"]
        else:
            params = {}
        return snappy_http.handle_http_query(db,
                                             event["requestContext"]["http"]["path"],
                                             params)
    else:
        snappy_mqtt_event(db, event, context)

# message_type is set up by MQTT SQL to be "startup" or "observation"

def snappy_mqtt_event(db, event, context):
    global mock_mqtt, mocked_outgoing
    if mock_mqtt:
        mocked_outgoing = []
    if event["message_type"] == "startup":
        responses = snappy_startup.handle_startup_event(db, event, context)
    else:
        responses = snappy_observation.handle_observation_event(db, event, context)
    if mock_mqtt:
        mocked_outgoing = responses
    else:
        for resp in responses:
            snappy_mqtt.publish(resp[0], resp[1], resp[2])
