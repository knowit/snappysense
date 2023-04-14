-*- fill-column: 100 -*-

# Setting up the back-end

The back-end comprises:

* Routing code that ensures incoming MQTT traffic is routed to lambda code, and that results from
  the lambda code is routed back to the device
* Lambda code to process MQTT traffic and HTTP queries from the front end
* DynamoDB tables to hold data about devices, device classes, locations, measurement factors, and 
  observations

This file explains how to set all of that up.

NOTE that the lambda, the database, and the IoT logic all have to be in the same AWS region, as all
explicit region names have been removed from that code.


## Routing messages from the Things to Lambdas

We must route incoming MQTT traffic to Lambda functions for processing.

There is a single Lambda function, `snappySense`, that responds to traffic from Things, but two
routes in AWS IoT to route traffic to it, one for startup messages and one for sensor observation
messages.  (The reason for having two routes is to avoid wildcards on the second token of the topic,
so as to avoid every chance of message loops.)

### Setting up the lambdas with test code

The lambda function has to be set up only once per sandbox and once for production; after that it is
easy to upload new code to it.

#### Creating a role

Every Lambda has a _role_, this is called `snappySense-lambdaRole`.  The Lambda's role has _policies_
that allow the Lambda to perform AWS actions: logging, DynamoDB access, and AWS IoT message
publishing.  The JSON for the role is in `snappy_lambda_role.json` in the present directory.

You only need to create the role once (per sandbox or production environment).  But it's a multi-step
process, first you create the policies, then the role.

First, make a local copy of `snappy_lambda_role.json` in which you replace my user ID (`92..76`)
with your own and the region ID (`eu-central-1`) with your own, if it differs.  Call this file
`my_snappy_lambda_role.json`.

Next, in AWS > Management console > Identity and Access Management (IAM) > Access Management > Policies:

* Click "Create policy"
* Click on the "JSON" tab
* Paste in the contents of the file `snappy_lambda_role.json`
* Click "Next: Tags" and then "Next: Review"
* For `Name`, enter `snappy-lambda-policy`, and if you like, "Policy for SnappySense lambda functions" 
  in the description field
* Check that the permissions look all right
* Click "Create policy"

Finally, in AWS > Management console > Identity and Access Management (IAM) > Access Management > Roles:

* Click "Create role"
* Select "AWS service" under Trusted entity type and "Lambda" under use case, press "Next"
* Select `snappy-lambda-role`
* Click "Next"
* Enter the name `snappy-lambda-role` and the description might be "Allows SnappySense Lambda
  functions to call AWS services on your behalf."
* Click "Create role"

TODO: How to do this with AWS CLI or something like `dbop`.

#### Creating a stub lambda functions

There is a single function `snappySense` to handle both startup and observation messages.

In AWS > Management console > Lambda, click "Create function" and give it these parameters:

* Name `snappySense`
* Python 3.9 
* Architecture doesn't matter but arm64 is cheaper so choose that
* Under "Change default execution role" choose "Use an existing role", then choose `snappy-lambda-role`.
* Click "Create function"

Next, you're in the function code view.

* Paste in the code from the file `test-code/echo_lambda.py`
* In the Runtime settings panel, set the handler name to `lambda_function.snappysense_event`
* Click "Deploy" to update the code

TODO: How to do this with AWS CLI or something like `dbop`.

#### Creating the routes and the SELECT statements

Finally we tie it together by routing MQTT traffic to the Lambda.

In AWS > Management console > IoT Core > Message Routing > Rules, create a new rule:

* The rule name should be `snappySenseStartup`
* The description should be something like "Route SnappySense startup messages to Lambda"
* The SQL (with version 2016-03-23) should be 
  ```
    SELECT *, topic(2) as message_type, topic(3) as class, topic(4) as device FROM 'snappy/startup/+/+'
  ```
* For the action, choose "Lambda" and then choose `snappySense` as the lambda function
* Click "Next" until you're done.

Now **repeat those steps** with the difference that the rule name is `snappySenseObservation`, the topic
filter is `snappy/observation/+/+`, and that "observation" replaces "startup" in the description.

(The reason for having two rules is to avoid a wildcard that might accidentally match a message
coming back from the Lambda, heading to a Thing.)

TODO: How to do this with AWS CLI or something like `dbop`.

#### Testing it all

Go to AWS > Management console > IoT Core.  Select "MQTT Test client".  Subscribe to '#'.  Go to the
Publish pane and publish any valid JSON you like to `snappy/startup/1/1`.  In the message log you
should see two messages, the one you sent and the echoed message.  The latter has three fields added
(message_type, class, and device).  Repeat the experiment for `snappy/observation/1/1`.

Once this test passes, you know that message routing works within AWS.


## Setting up the database tables

### The data model and the tables

There are five database tables.  Their fields and keys are defined in
[DATA-MODEL.md](DATA-MODEL.md), though being NoSQL the fields don't matter for database creation.

The `dbop` program (see later section) is used to initialize the five tables if they don't exist:

```
   dbop device create-table
   dbop class create-table
   dbop factor create-table
   dbop location create-table
   dbop observation create-table
```

Similarly `delete-table` can be used to delete tables that need to be cleaned out before creating
them afresh.

There is a test script, `test-code/snappysense-demo-data.sh`, that may be useful.

### Adding data to the database without any UI

The `dbop` program (see later section) is used to add data to the database and query it:
```
   dbop class add class=SnappySense 'description=SnappySense 3rd gen prototype device'
   dbop location add location=takterrassen 'description=Takterrassen i Universitetsgt 1'
   dbop location add location=akebakken 'description=Hjemmekontoret til Lars T'
   dbop device add device=snp_1_1_no_1 class=SnappySense location=takterrassen
   dbop device add device=snp_1_1_no_2 class=SnappySense location=akebakken
```

To list the contents of a table:
```
   dbop device list
```

To examine just one item in a table:
```
   dbop device get snp_1_1_no_2
```

### Testing that database access works from the lambda

Open the lambda console and paste in the code from `test-code/db_lambda.py` to replace the earlier
test code.  Deploy it.

In the MQTT console, subscribe to `#` if you haven't, then publish a message with the topic
`snappy/startup/1/1` and a body like this:

```
{ "query": "snp_1_1_no_1" }
```

If that device had earlier been entered into the database using the CLI, the contents of the record
will be echoed back at you in the console now.

## The `dbop` program

The `dbop` command line utility is a simple Go program that talks to AWS DynamoDB.  You need to have
`go` on your machine, which you can get from `https://golang.org`, your package manager, and in
other ways.

To build, go to `aws/dbop` and run `go build`.  The executable should appear in that directory.  It
is run as explained in earlier sections.

To run it, you must have set up your AWS config and credentials in the normal way for the AWS CLI,
either as environment variables or in the `~/.aws/config` and `~/.aws/credentials` files.

For some general help, run `dbop help`.


## Uploading the real Lambda code

Go to `lambda/`, then run `make` or duplicate the actions of the `Makefile` in that directory.


## Creating a Function URL for the Lambda

In AWS Lambda, click on the Configuration tab and the Function URL pane, it will allow you to create
a function URL.  Choose "None" as the security - security by obscurity rules!

## Testing the Function URL and the Lambda

Supposing that your function URL is `https://xxx.on.aws` and that you have added some device data to
the tables using dbop as described above, this should produce a list of devices:
```
  curl https://xxx.on.aws/devices
```

If you paste `https://xxx.on.aws` into a browser address bar, you should see a web application for
querying the database.
