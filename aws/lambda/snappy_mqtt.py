# -*- fill-column: 100 -*-
#
# MQTT stuff for SnappySense.
#
# See ../MQTT-PROTOCOL.md for a description of the messages.
# See ../DATA-MODEL.md for a description of the databases.

import boto3
import json

# NOTE That this assumes that the iot account is in the same region as this lambda.

def publish(topic, payload, qos):
    iot_client = boto3.client('iot-data')
    iot_client.publish(
        topic=topic,
        qos=qos,
        payload=json.dumps(payload)
    )
