-*- fill-column: 100 -*-

# AWS IoT backend for SnappySense

## High-level SnappySense network architecture

Each device in the SnappySense network can optionally report its observations to AWS IoT, where they
will be ingested and stored in a database for display and analysis.  Also optionally, AWS IoT can
transmit data back to the device to control it in various ways.

(Devices in the network can be SnappySense devices of various kinds but also sensors connected to
other types of devices; for simplicity I will mainly consider the standard SnappySense devices in
the following.)

Currently the only communication protocol between a device and AWS IoT is MQTT, a pub/sub protocol.
The device publishes observatins and subscribes to control messages.  The server subscribes to observations
and publishes control messages.  The full protocol is defined in MQTT-PROTOCOL.md.

Each device must be configured so that it can connect to AWS IoT: It must be provisioned with an AWS
"Thing" identity and the necessary certificates for that Thing.  Sections below describe how to
create a new Thing, obtain its identity documents, and install them on SnappySense devices.

SnappySense devices communicate by WiFi and need to be configured for the WiFi available at their
location; they also need to be given a location name appropriate for the location.  This end-user
process is described in [../firmware-arduino/MANUAL.md](../firmware-arduino/MANUAL.md).

On the backend, data are stored in a DynamoDB database.  The database is updated by a Lambda when
observations arrive at AWS IoT.

At present (February 2023) there is no convenient UI for database access.  All work must be done
from AWS consoles or from the AWS CLI.  In the future the UI must allow the registration of things,
locations, sensor types, and so on.  It must allow alerts to be viewed and stats to be shown.  All
of this is TBD.


## Creating a Thing and obtaining its identity documents

(This is suitable for a very small fleet of devices.  There are other methods suitable for larger fleets.)

Every device ("Thing") needs a unique _Thing Name_.  The format for a SnappySense Thing name shall
be `snp_x_y_no_z` for devices with hardware version `x.y` and serial number `z` within that version,
ie, `snp_1_2_no_37` has hardware v1.2 and serial number 37.  Non-SnappySense Things should use their
own, compact, class designators (eg `rpi` would make sense for Raspberry Pis).

Every Thing also has a _Thing Type_, this aids device management.  For SnappySense Things, the Thing
type is `SnappySense`.  Non-SnappySense Things should have Thing Types that are descriptive (eg
`RaspberryPi`); stick to letters, digits, `-` and `.`, or you'll be sorry.

In addition to being created in AWS IoT, the Thing must be registered in our backend databases; see
[DESIGN.md](DESIGN.md) and the "Creating a Thing" section below.

Every Thing is associated with a number of files containing secrets specific to the Thing:
certificates from AWS, and a configuration file containing these certificates along with WiFi
passwords and similar.  As a matter of hygiene, these files should all be stored together in a
directory that has the same name as the Thing.  Also see the sections below on "Factory
provisioning" and "Saving the secrets for later".

To create a new Thing in AWS IoT, follow the steps outlined below.  These steps will be the same
whether you are using the **Dataplattform production console** or your own **Sandbox console**.  If
the Thing's identity will be long-lived then you should _not_ create it in your sandbox.

### Creating a security policy

You need to create a security policy for SnappySense devices if it does not already exist.  It
should be called `SnappySensePolicy`.  The JSON for the policy is in the file `thing_policy.json` in
the current directory.

Go to AWS > Management console > IoT Core > Security > Policies.

To create the policy, click `Create Policy`, then select the JSON option and paste in the JSON from
`thing_policy.json`, then click to confirm creation.

All things in the SnappySense network, whether SnappySense things or not, can use the same policy.

TODO: How to do this with the AWS CLI or something like `dbop`, including checking whether it needs
to be done.

### Creating the Thing and obtaining its documents
  
Create a new directory somewhere on your PC that will hold the secrets for the new Thing.  This
directory should have the same name as the Thing you are creating.

Go to AWS > Management console > IoT Core > All devices > Things.  Then:

* Click on "Create things", select "Create single thing", click "Next"
* The Thing Name has the format `snp_x_y_no_z` as explained in more detail above
* The Thing Type is `SnappySense`.  (If you have to create the thing type first, then the only thing
  we care about is the name; the description can be blank or something like "SnappySense devices".)
* The Thing has no device shadow.
* Click "Next"
* Choose to auto-generate certificates; click "Next" again.
* Now you have to select a policy.  Choose `SnappySensePolicy`, created above.  If you did not
  create it already, create it now.
* Click "Create thing".
* You'll be presented with a download screen.  In the directory for the device, created above, store
  all five files.

Congratulations, you have a new Thing.  For information about how to install the documents on the
device and how to keep the secrets safe, see "Factory provisioning" and "Saving the secrets for
later", below.

TODO: How to do this with the AWS CLI or something like `dbop`.

### Registering a new Thing with the SnappySense backend

This is covered below under "Setting up the database tables".

### Some notes about the security policy (for the specially interested only)

The current (February 2023) security policy in `thing_policy.json` very lax.  It would be better to
restrict it to these actions only:

* `iot:Connect`
* `iot:Subscribe` for `snappy/control/+`
* `iot:Subscribe` for `snappy/command/+`
* `iot:Publish` for `snappy/startup/+/+`
* `iot:Publish` for `snappy/observation/+/+`.

The restrictions would be more secure, as it would make it impossible for Things to subscribe to
non-SnappySense messages flowing through the AWS MQTT broker.

Even better would be if a Thing would only be allowed to subscribe to those messages that are sent
to itself: this would require a separate policy per Thing however, and might not be practical in a
large fleet of devices.

## Routing messages from the Things to Lambdas

Now that we have Things that can publish and subscribe to messages, we must route incoming MQTT
traffic to Lambda functions for processing.

There is a single Lambda function, `snappySense`, that responds to traffic from Things, but two
routes in AWS IoT to route traffic to it, one for startup messages and one for sensor observation
messages.  (The reason for this is to avoid wildcards on the second token of the topic, so as to
avoid every chance of message loops.)

### Setting up the lambdas with test code

The lambda function has to be set up only once per sandbox and once for production; after that it is
easy to upload new code to it.

#### Creating a role

Every Lambda has a _role_, this is called `snappySense-lambdaRole`.  The Lambda's role has _policies_
that allow the Lambda to perform AWS actions: logging, DynamoDB access, and AWS IoT message
publishing.  The JSON for the role is in `snappy_lambda_role.json` in the present directory.

You only need to create the role once (per sandbox or production environment).  But it's a multi-step
process, first you create the policies, then the role.

First, make a local copy of `snappy_lambda_role.json` in which you replace my user ID (`92..76`) with
your own.  Call this file `my_snappy_lambda_role.json`.

Next, in AWS > Management console > Identity and Access Management (IAM) > Access Management > Policies:

* Click "Create policy"
* Click on the "JSON" tab
* Paste in the contents of the file `my_snappy_lambda_role.json`
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

* Paste in the code from the file `echo_lambda.py` in the present directory
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
Publish pane and publish anything you like to `snappy/startup/1/1`.  In the message log you should
see two messages, the one you sent and the echoed message, with a three fields added (message_type,
class, and device).  Repeat the experiment for `snappy/observation/1/1`.

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

TODO: Link to the snappysense-demo-data.sh script here.

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

Open the lambda console and paste in the code from `db_lambda.py` in the present directory to
replace the earlier test code.  Deploy it.

In the MQTT console, subscribe to `#` if you haven't, then publish a message with the topic
`snappy/startup/1/1` and a body like this:

```
{ "query": "snp_1_1_no_1" }
```

If that device had earlier been entered into the database using the CLI, the contents of the record
will be echoed back at you in the console now.

## Factory provisioning a SnappySense device

Start by making a copy of `firmware-arduino/src/configuration_template.txt` into a new file in the
directory `snp_x_y_no_z`.  I find it useful to call this file `snp_x_y_no_z.cfg`.  (Or copy a file
you already have, but be careful to update the AWS ID and certs in the file.)  The format of this
file is described in detail in [../CONFIG.md](../CONFIG.md).

_This file will contain secrets and must not be checked into github._

Add the necessary information to the new file as described in the comments.  The `aws-iot-id` shall
be the name of the device, `snp_x_y_no_z`, as chosen above.  The device certificate, private key,
and root cert must be the ones downloaded.

The WiFi information in the config file is optional (all three networks) and can be configured by
the end-user.  However it may be useful to configure one network here to allow for factory testing.

The location information is also optional and can be configured by the end-user.

Once the file is complete, save it.

Upload the configuration file to the device as described in [../CONFIG.md](../CONFIG.md), then view
the installed config to check it, also described in that document.


## Saving the secrets for later (IMPORTANT!)

The configuration file contains secrets and will need to be stored securely.  Also, some of the
secrets can't be downloaded from AWS more than the once.  We want to keep both the config file and
the certificate files in a safe place for later use.  Follow this procedure:

In the parent directory of `snp_x_y_no_z`, create a zip file containing the configuration:
```
  zip -r snp_x_y_no_z.zip snp_x_y_no_z/
```

Now **upload that zip file to 1Password** as a new "Document" type with the title `AWS IoT snp_x_y_no_z`.
If there's anything to note, add it to the Notes field in 1Password.

If the cert was created under the Dataplattform **production** account then use the Dataplattform
1Password account; if you used your AWS sandbox account, then upload the file to your own 1Password
account.

## The `dbop` program

The `dbop` command line utility is a simple Go program that talks to AWS DynamoDB.  You need to have
`go` on your machine, which you can get from `https://golang.org`, your package manager, and in
other ways.

To build, go to `aws-dynamodb/dbop` and run `go build`.  The executable should appear in that
directory.  It is run as explained in earlier sections.

To run it, you must have set up your AWS credentials in the normal way for the AWS CLI, either as
environment variables or in a ~/.aws/credentials file.

For some general help, run `dbop help`.
