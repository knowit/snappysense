# TODO

High pri

* Live test lambda_handler.py also
* Document how to upload code for the lambda, and test that the lambda works by sending mqtt to it,
  then checking the database
* Fix the firmware so that it sends the location with the reading
* Live test the devices

Then in fairly random order:

* Rename cli as 'snappydb'
* moto test cases - revamp, test other things?
* live test cases - use assertions, not print?

Longer term:

* the CLI could have some new operations:
  * send-startup device=... would mqtt send a message, requires device credentials
  * send-reading device=... ditto


