-*- fill-column: 100 -*-

# AWS Lambda front-end for SnappySense

This maintains the SnappySense databases jointly with the back-end (see ../aws-iot-backend), and
allows data to be queried.  See [../aws-iot-backend/README.md](../aws-iot-backend/README.md) and
other files in that directory for information about the data model and so on.

During development it's possible to test many things against the test server in ../test-server.

## TODO (lots of things)

- point data (motion) are not usefully plotted on a line plot?
- be able to deselect in the multiselects without reloading page
- probably want to rename the lambda from snappyStartup to something else...
- better labeling of the plot
- select date range
