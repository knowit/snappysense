# This must serve these requests:
#
#  /            - index.html
#  /index.html  - index.html
#  /snappy.js   - snappy.js
#  /devices     - list of device records
#  /locations   - list of location records
#  /factors     - list of factors
#  /observations_by_device_and_factor?device=nnn&factor=mmm - list of [sent,value]
#  /devices_by_location?location=nnn - list of device records

import snappy_data

def handle_http_query(db, path, queryParameters):
    if path == "/" or path == "/index.html":
        bodyf = open("index.html")
        return {
            "statusCode": 200,
            "headers": {"Content-Type": "text/html"},
            "body": bodyf.read() }

    if path == "/snappy.js":
        bodyf = open("snappy.js")
        return {
            "statusCode": 200,
            "headers": {"Content-Type": "application/javascript"},
            "body": bodyf.read() }

    if path == "/devices":
        return {
            "statusCode": 200,
            "headers": {},
            "body": snappy_data.list_devices(db) }

    if path == "/locations":
        return {
            "statusCode": 200,
            "headers": {},
            "body": snappy_data.list_locations(db) }

    if path == "/factors":
        return {
            "statusCode": 200,
            "headers": {},
            "body": snappy_data.list_factors(db) }

    if (path == '/observations_by_device_and_factor' and
        "device" in queryParameters and
        "factor" in queryParameters):
        device = queryParameters["device"]
        factor = queryParameters["factor"]
        return {
            "statusCode": 200,
            "headers": {},
            "body": snappy_data.query_observations_by_device_and_factor(
                db, device, factor) }

    if path == "/devices_by_location" and "location" in queryParameters:
        location = queryParameters["location"]
        return {
            "statusCode": 200,
            "headers": {},
            "body": snappy_data.query_devices_by_location(db, location) }

    return { "statusCode": 403 }

