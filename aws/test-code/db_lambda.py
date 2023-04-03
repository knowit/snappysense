# This is a stub AWS lambda to test the snappysense IoT database access.  It receives a message sent
# to it asking for information about a device, and responds with that information to the topic
# 'snappy/device'.  You should be able to watch message traffic in both directions in the MQTT Console
# in AWS IoT.
#
# For extensive instructions on where this comes in, see README.md

import json
import boto3

# NOTE this must be in the same region as the dynamodb and iot accounts.

# Event fields
#  "query": string, device-id

def snappysense_event(event, context):
    db = boto3.client('dynamodb')
    iot_client = boto3.client('iot-data')

    device_name = event["query"]
    probe = db.get_item(TableName='snappy_device', Key={"device":{"S": device_name}})
    if probe == None or "Item" not in probe:
        result = {"result": "FAILURE"}
    else:
        result = {"result": "SUCCESS", "info": probe["Item"]}

    iot_client.publish(
        topic="snappy/device",
        qos=1,
        payload=json.dumps(result)
    )
