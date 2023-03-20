-*- fill-column: 100 -*-

# Ad-hoc MQTT server

This is a simple server that connects to the MQTT broker and listens for startup and observation
messages.  Configure it, if necessary, by changing constants at the head of the program and
recompiling.

It assumes that the MQTT broker is set up with a username/password access method; TLS is possible
but since this is for local testing I'm not using it.

When a startup message is received we record it in the startup log and send a synthesized control
message for the device back to the broker (just to make sure that that works).  We also record the
received message in STARTUP.log.

When an observation is received we append it to OBSERVATION.log.

The broker is a separate thing, not included here.  I've been using RabbitMQ with its MQTT plugin
and the default "guest/guest" account for the server; note you must also configure the client with
the appropriate credentials.

(The Makefile contains information about how to configure RabbitMQ to run in a docker container.)

Here's a simple device factory config that might work for you if you add info for your WiFi network
and adapt it to the correct device version and so on:

```
version 2.0.0

# lth's home WiFi
set ssid1 radiolux
set password1 SuperSecret

# MQTT overall configuration
set mqtt-id rabbitmq_test
set mqtt-class snappysense

# For rabbitmq running on lth's laptop "corto"
set mqtt-use-tls 0
set mqtt-auth pass
set mqtt-endpoint-host corto.local
set mqtt-endpoint-port 1883
set mqtt-username lth
set mqtt-password qumquat
```

(Another obvious broker option is Mosquitto, but I've not tested that.)
