# -*- fill-column: 100 -*-
#
# Front-end support.
#
# This must serve
#  /            - index.html
#  /index.html  - index.html
#  /snappy.js   - snappy.js
#  /devices     - list of device records
#  /locations   - list of location records
#  /factors     - list of factors
#  /observations_by_device_and_factor?device=nnn&factor=mmm - list of [sent,value]
#  /devices_by_location?location=nnn - list of device records
#
# Only for /observations_by_device_and_factor should we worry about volume

import boto3
import json

def snappy_query(event, context):
    is_http = False
    if "requestContext" in event and "http" in event["requestContext"]:
        is_http = True
        h = event["requestContext"]["http"]
        p = h["path"]
    else:
        p = event["the_query"]

    if p == "/" or p == "/index.html":
        bodyf = open("index.html")
        return {
            "statusCode": 200,
            "headers": {"Content-Type": "text/html"},
            "body": bodyf.read() }

    if p == "/snappy.js":
        bodyf = open("snappy.js")
        return {
            "statusCode": 200,
            "headers": {"Content-Type": "application/javascript"},
            "body": bodyf.read() }

    db = boto3.client('dynamodb', region_name='eu-central-1')
    if p == "/devices":
        return {
            "statusCode": 200,
            "headers": {},
            "body": list_devices(db) }

    if p == "/locations":
        return {
            "statusCode": 200,
            "headers": {},
            "body": list_locations(db) }

    if p == "/factors":
        return {
            "statusCode": 200,
            "headers": {},
            "body": list_factors(db) }

    if p == '/observations_by_device_and_factor':
        if is_http:
            device = event["queryStringParameters"]["device"]
            factor = event["queryStringParameters"]["factor"]
        else:
            device = event["device"]
            factor = event["factor"]
        return {
            "statusCode": 200,
            "headers": {},
            "body": query_observations_by_device_and_factor(db, device, factor) }

    if p == "/devices_by_location":
        if is_http:
            location = event["queryStringParameters"]["location"]
        else:
            location = event["location"]
        return {
            "statusCode": 200,
            "headers": {},
            "body": query_devices_by_location(db, location) }

    return { "statusCode": 403 }

# Returns a list of observations, each observation is itself a list:
#   [sent, value]
# where `value` is the value of the field with name F#<factor> and `sent` is the value of the `sent`
# field of the observation.  Both values are numbers.

def query_observations_by_device_and_factor(db, device, factor):
    field = "F#" + factor
    response = db.scan(
        TableName='snappy_observation',
        ExpressionAttributeNames={
            '#FAC': field,
            '#SEN': 'sent',
        },
        ExpressionAttributeValues={
            ':d': {
                'S': device,
            },
        },
        FilterExpression='device = :d',
        ProjectionExpression='#SEN, #FAC')
    result = []
    # Not real happy about the use of float() here, the conversion is really
    # field-specific.  But if there's JS or Python on the other end of the
    # pipeline this will be OK.
    for d in response["Items"]:
        result.append([int(d["sent"]["N"]), float(d[field]["N"])])
    return result

def query_devices_by_location(db, location):
    response = db.scan(
        TableName='snappy_device',
        ExpressionAttributeNames={
            '#LOC':'location',
            '#DEV':'device',
        },
        ExpressionAttributeValues={
            ':l': {
                'S': location,
            },
        },
        FilterExpression='#LOC = :l',
        ProjectionExpression='#DEV, #LOC')
    return format_devices(response)

def list_devices(db):
    response = db.scan(
        TableName='snappy_device',
        ExpressionAttributeNames={
            '#DEV': 'device',
            '#LOC': 'location',
        },
        ProjectionExpression='#DEV, #LOC')
    return format_devices(response)
    
def format_devices(response):
    result = []
    for l in response["Items"]:
        result.append({'location':l['location']['S'], 
                       'device':l['device']['S']})
    return result

def list_locations(db):
    response = db.scan(
        TableName='snappy_location',
        ExpressionAttributeNames={
            '#LOC': 'location',
            '#DES': 'description',
        },
        ProjectionExpression='#LOC, #DES')
    result = []
    for l in response["Items"]:
        result.append({'location':l['location']['S'], 
                       'description':l['description']['S']})
    return result

def list_factors(db):
    response = db.scan(
        TableName='snappy_factor',
        ExpressionAttributeNames={
            '#FAC': 'factor',
            '#DES': 'description',
        },
        ProjectionExpression='#DES, #FAC')
    result = []
    for l in response["Items"]:
        result.append({'factor':l['factor']['S'], 
                       'description':l['description']['S']})
    return result
