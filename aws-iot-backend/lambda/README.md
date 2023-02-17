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
  zip snappy.zip lambda_function.py snappy_*.py
  aws lambda update-function-code --function-name snappySense --zip-file fileb://$(pwd)/snappy.zip
```
