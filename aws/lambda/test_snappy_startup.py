# -*- fill-column: 100 -*-

import boto3
from moto import mock_dynamodb

# Moto appears basically broken - every attempt to compartementalize any of this logic has failed,
# whether with fixtures or with annotated subroutines.  So for now it's all in one function.

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
    import snappy_data

    #
    # TEST: Simple startup event with default reading interval.
    #
    # This should work but there should be no response, as the device remains enabled and the
    # reading interval is the default.
    #

    responses = snappy_startup.handle_startup_event(
        dynamodb,
        {"device":           "1",
         "class":            "RPi2B+",
         "time":             "2023-02-05T16:55/sun",
         "reading_interval": snappy_data.DEFAULT_READING_INTERVAL},
        {})
    assert len(responses) == 0
    
    #
    # TEST: Startup event with subsequent control message for reading interval.
    #
    # A control message should be generated that changes the reading interval for this device, since
    # the interval the device reports differs from the default in the database.
    #

    assert 4 != snappy_data.DEFAULT_READING_INTERVAL
    responses = snappy_startup.handle_startup_event(
        dynamodb,
        {"device":           "1",
         "class":            "RPi2B+",
         "time":             "2023-02-05T16:55/sun",
         "reading_interval": 4},
        {})

    assert len(responses) == 1
    r0 = responses[0]
    assert len(r0) == 3
    assert r0[0] == "snappy/control/RPi2B+/1"
    payload = r0[1]
    assert "reading_interval" in payload
    assert payload["reading_interval"] == snappy_data.DEFAULT_READING_INTERVAL
    assert r0[2] == 1
    
    #
    # TEST: Startup even with subsequent control message for disablement.
    #
    # There should be a control message that disables the device because it is marked in the DB as
    # disabled.
    #

    responses = snappy_startup.handle_startup_event(
        dynamodb,
        {"device":           "3",
         "class":            "MBPM1Max",
         "time":             "2023-02-05T16:55/sun",
         "reading_interval": 0},
        {})
    assert len(responses) == 1
    r0 = responses[0]
    assert len(r0) == 3
    assert r0[0] == "snappy/control/MBPM1Max/3"
    payload = r0[1]
    assert "enabled" in payload
    assert payload["enabled"] == 0
    assert r0[2] == 1
