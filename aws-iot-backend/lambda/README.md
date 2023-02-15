-*- fill-column: 100 -*-

# SnappySense back-end (AWS Lambda)

This is prototype AWS Lambda code for the SnappySense back-end.

`snappy_data.py` and `snappy_mqtt.py` make up the lower layer; these communicate with AWS dynamodb and
AWS iot-data.

Application logic is in `snappy_startup.py`, which handles `snappy/startup/+/+` events (device
startup messages), and in `snappy_reading.py`, which handles `snappy/reading/+/+` events (device
reading messages).

See various `*.md` files in the parent directory for design documents.

To run application logic regression tests, execute in this directory:
```
python3 -m pytest
```

To upload,

```
  rm -f snappy.zip
  zip snappy.zip snappy_*.py
  aws update-function-code --function-name snappySense --zip-file fileb://$(pwd)/snappy.zip
```

## TODO (lots of yak shaving!)

### Phase 1 (done)

* (done) Fix the policy: The current policy is set up for two functions, `snappyStartup` and
  `snappyReading`, but instead I think we'll have just one, `snappySense`, with a Python function
  `snappysense_event()` to take care of it, so all of that needs to change.
* (done) Implement a prototype `snappySense` function to MQTT-echo the input
* (done) Implement a route from MQTT to a `snappySense` lambda function
* (done) Test that all of that works (no DB access obviously but MQTT and Lambda)

### Phase 2 (done)

* (done) Who creates the DB tables and when does that happen?
* (done) How are new Things added to the DB if we don't have a UI?
* (done) Test that DB access works without running the snappy_reading code yet.  To do this, accept
  an MQTT msg that has a device ID, and echo back information about it.

### Phase 3

* BUG: There's no reason for DEVICE to have a secondary key.
* BUG: the data model does not specify what the JSON structures look like.  To wit, the **test
  programs** and the **snappy_data code** uses a generic List for the sensors and actuators while I
  chose a String List for some of the **test code** in the documentation.  This is not going to end
  well.
* BUG: `snappy_reading.py`: The snappy_reading.py code is not au courant with the current device
  firmware.  The firmware sends many sensor readings in a single package, while the code expects
  just one pair of `factor` and `reading` fields.
* BUG: `snappy_reading.py`: The actuator message looks like it has the wrong syntax.
* BUG: db.get_item requires both keys, so how this code passes tests I don't know - maybe it never needs to
  access the device table.  (Indeed it is so.  Bad.)
* BUG: Should try to fix the db_lambda.py too.
* BUG: Why is the field within DEVICE called `sensors` and not `factors`?
* BUG: Ditto in `location`
* Document how to upload code for the lambda, and test that this works

### Phase 4 or later

* What else needs to be added to the DB?  So far, snappy_data.py uses LOCATION, DEVICE, and HISTORY.
* Everything to do with the CLI
