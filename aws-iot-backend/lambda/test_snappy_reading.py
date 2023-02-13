# -*- fill-column: 100 -*-

import boto3
from moto import mock_dynamodb

# Cloned from test_snappy_startup, q.v.

@mock_dynamodb
def test_startup():
    dynamodb = boto3.client('dynamodb', region_name='eu-central-1')

    # Create and initialize LOCATION

    dynamodb.create_table(
        AttributeDefinitions=[
            {"AttributeName":"location", "AttributeType":"S"}
        ],
        TableName = 'snappy_location',
        KeySchema=[
            {"AttributeName":"location", "KeyType":"HASH"}
        ],
        BillingMode="PAY_PER_REQUEST"
    )
    dynamodb.put_item(TableName='snappy_location',
                      Item={"location": {"S": "lars-at-home"},
                            "sensors": {"L": [{"S": "1"}, {"S": "3"}]},
                            "actuators": {"L": [{"M": {"factor": {"S":"temperature"},
                                                       "device": {"S":"2"},
                                                       "idealfn": {"S":"constant/21"}}}]}})

    # Create and initialize DEVICE

    dynamodb.create_table(
        AttributeDefinitions=[
            {"AttributeName":"device", "AttributeType":"S"},
        ],
        TableName = 'snappy_device',
        KeySchema=[
            {"AttributeName":"device", "KeyType":"HASH"}
        ],
        BillingMode="PAY_PER_REQUEST"
    )
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"1",},
                            "class":{"S":"RPi2B+"},
                            "location":{"S":"lars-at-home"},
                            "sensors":{"L":[{"S":"temperature"}]},
                            "actuators":{"L":[]}})
    # "sensors":[], "actuators":["temperature"]
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"2"},
                            "class":{"S":"Thermostat"},
                            "location":{"S":"lars-at-home"},
                            "sensors":{"L":[]},
                            "actuators":{"L":[{"S":"temperature"}]}})
    dynamodb.put_item(TableName='snappy_device',
                      Item={"device":{"S":"3",},
                            "class":{"S":"MBPM1Max"},
                            "location":{"S":"lars-at-home"},
                            "enabled":{"N":"0"},
                            "sensors":{"L":[{"S":"temperature"}]},
                            "actuators":{"L":[]}})

    # Create and initialize HISTORY

    dynamodb.create_table(
        AttributeDefinitions=[
            {"AttributeName":"device", "AttributeType":"S"},
        ],
        TableName='snappy_history',
        KeySchema=[
            {"AttributeName":"device", "KeyType":"HASH"}
        ],
        BillingMode="PAY_PER_REQUEST"
    )

    # I don't know if this needs to be here but to be conservative, it is.

    import snappy_startup
    import snappy_reading
    import snappy_data

    #
    # SETUP.
    #
    # We need some startup messages to ensure that there are history elements for the devices
    # we test, this is part of the protocol.  The code should be resilient in the face of a failure
    # in this, but that should be tested separately.
    #
    # (Startup is also tested more thoroughly by test_snappy_startup.py.)
    #

    responses = snappy_startup.handle_startup_event(
        dynamodb,
        {"device":           "1",
         "class":            "RPi2B+",
         "time":             "2023-02-03T16:55/fri",
         "reading_interval": 0},
        {})

    responses = snappy_startup.handle_startup_event(
        dynamodb,
        {"device":           "2",
         "class":            "Thermostat",
         "time":             "2023-02-04T16:55/sat",
         "reading_interval": 0},
        {})

    #
    # TEST: Simple reading without consequent action.
    #
    # This should work but there should be no response, as the reading is already at
    # the ideal for the location.
    #

    h1 = snappy_data.get_history_entry(dynamodb, "1")
    assert snappy_data.history_last_contact(h1) == "2023-02-03T16:55/fri"
    r1 = snappy_data.history_readings(h1)
    assert len(r1) == 0

    responses = snappy_reading.handle_reading_event(
        dynamodb,
        {"device":  "1",
         "class":   "RPi2B+",
         "time":    "2023-02-05T16:55/sun",
         "factor":  "temperature",
         "reading": 21},
        {})
    assert len(responses) == 0

    h1 = snappy_data.get_history_entry(dynamodb, "1")
    assert snappy_data.history_last_contact(h1) == "2023-02-05T16:55/sun"
    r1 = snappy_data.history_readings(h1)
    assert len(r1) == 1
    assert r1[0]["temperature"] == 21
    a1 = snappy_data.history_actions(h1)
    assert len(a1) == 0

    # Device #2 is the only actuator at the given location.  Its actions list should still be empty,
    # as the temperature was at the ideal, and it should not have been marked as contacted during
    # the last event.

    h2 = snappy_data.get_history_entry(dynamodb, "2")
    assert snappy_data.history_last_contact(h2) < "2023-02-05T16:55/sun"
    r2 = snappy_data.history_readings(h2)
    assert len(r2) == 0
    a2 = snappy_data.history_actions(h2)
    assert len(a2) == 0

    #
    # TEST: Reading with consequent action.
    #
    # Here we should have a response to change the temperature, since the reading differs from the
    # ideal at the location.  The action should be recorded in the action list for the temperature
    # actuator at the location, not in the action list for the sensor.
    #

    responses = snappy_reading.handle_reading_event(
        dynamodb,
        {"device":  "1",
         "class":   "RPi2B+",
         "time":    "2023-02-06T16:55/mon",
         "factor":  "temperature",
         "reading": 19},
        {})
    assert len(responses) == 1

    h1 = snappy_data.get_history_entry(dynamodb, "1")
    assert snappy_data.history_last_contact(h1) == "2023-02-06T16:55/mon"
    r1 = snappy_data.history_readings(h1)
    assert len(r1) == 2
    assert r1[0]["temperature"] == 19
    assert r1[1]["temperature"] == 21
    a1 = snappy_data.history_actions(h1)
    assert len(a1) == 0

    h2 = snappy_data.get_history_entry(dynamodb, "2")
    assert snappy_data.history_last_contact(h2) == "2023-02-06T16:55/mon"
    r2 = snappy_data.history_readings(h2)
    assert len(r2) == 0
    a2 = snappy_data.history_actions(h2)
    assert len(a2) == 1
    assert "temperature" in a2[0]
    assert "time" in a2[0]
    assert "ideal" in a2[0]
    assert a2[0]["temperature"] == 19
    assert a2[0]["ideal"] == 21
    assert a2[0]["time"] == "2023-02-06T16:55/mon"
