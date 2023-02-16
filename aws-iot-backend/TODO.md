# TODO

## Now

* BUG: Most tables have changed quite a bit.  The OBSERVATIONS nee HISTORY table has changed the
  most; a lot of logic has to be rewritten here.
* BUG: `snappy_reading.py`: The snappy_reading.py code is not au courant with the current device
  firmware.  The firmware sends many sensor readings in a single package, while the code expects
  just one pair of `factor` and `reading` fields.
* BUG: `snappy_reading.py`: The actuator message looks like it has the wrong syntax.  Anyway this has
  been removed now, so remove it

## Later

* Document how to upload code for the lambda, and test that this works
* Test cases for lambda:
  * must test more operations, notably db.get_item, possibly others, ideally "everything"
* What else needs to be added to the DB?  So far, snappy_data.py uses LOCATION, DEVICE, and HISTORY.
* Everything to do with the CLI
