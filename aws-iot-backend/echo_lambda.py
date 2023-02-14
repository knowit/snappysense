# This is a stub AWS lambda to test the snappysense IoT message routing and permissions.  It
# receives all messages sent to it and echoes them back to the topic 'snappy/echo'.  You should be
# able to watch message traffic in both directions in the MQTT Console in AWS IoT.
#
# For extensive instructions on where this comes in (along with the # json files in this directory),
# see README.md

import json
import boto3

def snappysense_event(event, context):
    iot_client = boto3.client('iot-data', region_name='eu-central-1')
    iot_client.publish(
        topic="snappy/echo",
        qos=1,
        payload=json.dumps(event)
    )
