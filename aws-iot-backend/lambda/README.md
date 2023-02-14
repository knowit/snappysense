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

### Phase 2

* Who creates the DB tables and when does that happen?
* How are new Things added to the DB if we don't have a UI?
* What else needs to be added to the DB?  So far, snappy_data.py uses LOCATION, DEVICE, and HISTORY.
* Test that DB access works without running the snappy_reading code yet

### Phase 3

* `snappy_reading.py`: The snappy_reading.py code is not au courant with the current device
  firmware.  The firmware sends many sensor readings in a single package, while the code expects
  just one pair of `factor` and `reading` fields.
* `snappy_reading.py`: The actuator message from looks like it has the wrong syntax.
* Upload code

### Phase 3

* Everything to do with the CLI
