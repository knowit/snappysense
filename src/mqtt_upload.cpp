// Code for uploading data to a remote MQTT broker and receiving messages we subscribe to.
//
// Summarized from the design document:
//
// On startup, we publish to topic snappy/startup/<device-class>/<device-id> with
// fields "time" (timestamp, string) and "interval" (reading interval, positive integer seconds).
//
// On a reading, we publish to topic snappy/reading/<device-class>/<device-id> with
// all the fields in the sensor object.
//
// We subscribe to two topics:
//
// A message on snappy/control/<device-id> can have two fields, "enable" (0 or 1),
// and "interval" (reading interval, positive integer seconds).
//
// A message on snappy/command/<device-id> has three fields, "actuator" (the environment
// factor we want to control, string, this should equal one of the json keys for the
// sensor object), "reading" (numeric, a value representing what the server thinks the
// current state of that factor is), and "ideal" (numeric, a value representing what
// the server thinks the factor should be).

#include "mqtt_upload.h"
#include "config.h"
#include "log.h"
#include "network.h"
#include "snappytime.h"

#include <ArduinoMqttClient.h>
#include <WiFiClientSecure.h>

#ifdef MQTT_UPLOAD

// The default buffer size is 256 bytes on most devices.
static const size_t MQTT_BUFFER_SIZE = 1024;

void upload_results_to_mqtt_server(const SnappySenseData& data) {
  static bool mqtt_startup_sent = false;

  String topics[2];
  String messages[2];
  int msg_ptr = 0;

  if (!mqtt_startup_sent) {
    String& topic = topics[msg_ptr];
    String& body = messages[msg_ptr];
    msg_ptr++;

    topic += "snappy/startup/";
    topic += mqtt_device_class();
    topic += "/";
    topic += mqtt_device_id();

    body += "{\"interval\":";
    body += sensor_poll_frequency_seconds();
    body += ",\"time\":\"";
    body += format_time(snappy_local_time());
    body += "\"}";

    mqtt_startup_sent = true;
  }

  // Number 0 is mostly bogus data so skip it
  if (data.sequence_number > 0) {
    String& topic = topics[msg_ptr];
    String& body = messages[msg_ptr];
    msg_ptr++;

    topic += "snappy/reading/";
    topic += mqtt_device_class();
    topic += "/";
    topic += mqtt_device_id();

    body = format_readings_as_json(data);
  }

  if (msg_ptr > 0) {
    // TODO: This is not great: we probably want a more flexible idea of whether the
    // wifi connection stays up or not for mqtt.  Also see comments in main.cpp.
    auto holder(connect_to_wifi());

    WiFiClientSecure client;
    client.setCACert(mqtt_root_ca_cert());
    client.setCertificate(mqtt_device_cert());
    client.setPrivateKey(mqtt_device_private_key());

    // Here the buffer size should really be the max of the lengths of the messages we're trying to send.
    // Not sure how the length of the topic may fit into that.
    MqttClient clientMQTT(client);
    clientMQTT.setId(mqtt_device_id());
    clientMQTT.setTxPayloadSize(MQTT_BUFFER_SIZE);

    // Connect to the MQTT broker on the AWS endpoint
    log("Mqtt: Connecting to AWS IOT ");
    while (!clientMQTT.connect(mqtt_endpoint_host(), mqtt_endpoint_port())) {
      log(".");
      delay(100);
    }
    if(!clientMQTT.connected()){
      log("AWS IoT Timeout!\n");
      return;
    }  
    log("Connected!\n");

    for ( int i=0 ; i < msg_ptr; i++ ) {
      if (messages[i].length() > MQTT_BUFFER_SIZE) {
        log("Mqtt: Message too long!\n");
        continue;
      }

      clientMQTT.beginMessage(topics[i].c_str(), false, 1, 0);
      if (clientMQTT.write((uint8_t*)messages[i].c_str(), messages[i].length()) != messages[i].length()) {
        // I think this shouldn't happen since we checked above, but
        // it's not totally clear that it won't.  It's possible the
        // topic is also in the buffer, and there may be metadata too.
        log("Mqtt: Message was chopped!\n");
      }
      clientMQTT.endMessage();
    }

    delay(100);
    client.stop();
  }
}

#endif
