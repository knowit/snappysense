-*- fill-column: 100 -*-

# SnappySense back-end and front-end (AWS Lambda)

This is prototype AWS Lambda code for the SnappySense back-end and front-end.

`snappy_data.py` and `snappy_mqtt.py` make up the lower layer; these communicate with AWS dynamodb and
AWS iot-data.

Application logic for MQTT ingest and command generation is in `snappy_startup.py`, which handles
`snappy/startup/+/+` events (device startup messages), and in `snappy_observation.py`, which handles
`snappy/observation/+/+` events (device observation messages).

Application logic for the front end is in `snappy_http.py`, which handles HTTP requests, and in
`index.html` and `snappy.js`, which are served by that logic.

See various `*.md` files in the parent directory for design documents.

To upload, either run `make` in this directory or duplicate the actions in Makefile.
