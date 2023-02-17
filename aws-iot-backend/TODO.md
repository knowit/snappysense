# TODO

High pri

* Document how to upload code for the lambda, somewhere findable
* Why is the table called 'observations' and not 'observation', when the table for devices is called
  'device'?
* Rename 'reading' as 'observation', this is going to hurt, it affects a lot
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


