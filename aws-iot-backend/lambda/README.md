-*- fill-column: 100 -*-

# SnappySense back-end (AWS Lambda)

This is prototype AWS Lambda code for the SnappySense back-end.

`snappy_data.py` and `snappy_mqtt.py` make up the lower layer; these communicate with AWS dynamodb and
AWS iot-data.

Application logic is in `snappy_startup.py`, which handles `snappy/startup/+/+` events (device
startup messages), and in `snappy_observation.py`, which handles `snappy/observation/+/+` events
(device observation messages).

See various `*.md` files in the parent directory for design documents.

To run application logic regression tests, execute in this directory:
```
python3 -m pytest
```

To upload, either run `make` in this directory or duplicate the actions in Makefile.
