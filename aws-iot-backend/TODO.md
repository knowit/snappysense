# TODO

High pri

* Document how to upload code for the lambda, somewhere findable
* Fix the firmware so that it uses the topic 'snappy/observation/...'
* Fix the firmware so that it sends the location with the observation
* Live test the devices

Then in fairly random order:

* Rename cli as 'snappydb'
* moto test cases - revamp, test other things?
* live test cases - use assertions, not print?

Longer term:

* the CLI could have some new operations:
  * send-startup device=... would mqtt send a message, requires device credentials
  * send-observation device=... ditto


