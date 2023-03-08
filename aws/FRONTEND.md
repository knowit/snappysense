-*- fill-column: 100 -*-

# Notes about the SnappySense front-end

The front-end currently consists of hand-written HTML and JavaScript that talks to the lambda to
receive static assets and query results.  (There is currently no way to update data from the
front-end.)  Simply load the root document at the Function URL for the lambda to be served the
application.

The application has some selection boxes where one can select devices by ID or by location, and then
the factor to display and the time range of interest.  Pressing `Query` will then display the
results of the query in the plot.

This is all fairly primitive, but we don't need anything else right now.

## TODO (lots of things, many more than this)

- better labeling of the plot
- point data (motion) are not usefully plotted on a line plot
- be able to deselect in the multiselects without reloading page
