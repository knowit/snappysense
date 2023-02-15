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

### Phase 1 (clean up AWS documentation, part 1) (done)

### Phase 2 (clean up AWS documentation, part 2) (done)

### Phase 3 (make it all hang together)

* BUG: `snappy_reading.py`: The snappy_reading.py code is not au courant with the current device
  firmware.  The firmware sends many sensor readings in a single package, while the code expects
  just one pair of `factor` and `reading` fields.
* BUG: `snappy_reading.py`: The actuator message looks like it has the wrong syntax.

### Phase 4 or later

* Document how to upload code for the lambda, and test that this works
* Test cases for lambda:
  * must test more operations, notably db.get_item, possibly others, ideally "everything"
* What else needs to be added to the DB?  So far, snappy_data.py uses LOCATION, DEVICE, and HISTORY.
* Everything to do with the CLI
