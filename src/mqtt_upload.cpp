// Code for uploading data to a remote MQTT broker.

#include "mqtt_upload.h"
#include "config.h"
#include "log.h"
#include "network.h"

#include <ArduinoMqttClient.h>
#include <WiFiClientSecure.h>

#ifdef MQTT_UPLOAD

#define MQTT_VERBOSE

// The default buffer size is 256 bytes on most devices.
static const size_t MQTT_BUFFER_SIZE = 1024;

void upload_results_to_mqtt_server(const SnappySenseData& data) {
  if (data.sequence_number == 0) {
    // Mostly bogus data
    return;
  }

  // The message is currently on the large side, it should be shorter.
  // TODO: Is it possible to not have a static buffer size here but to
  // base the buffer size on the message length?  That would be neat.
  String msg = format_readings_as_json(data);
  if (msg.length() > MQTT_BUFFER_SIZE) {
    log("Mqtt: Message too long!\n");
    return;
  }

  // TODO: This is not great: we probably want a more flexible idea of whether the
  // wifi connection stays up or not for mqtt.  Also see comments in main.cpp.
  auto holder(connect_to_wifi());

  WiFiClientSecure client;
  client.setCACert(mqtt_root_ca_cert());
  client.setCertificate(mqtt_device_cert());
  client.setPrivateKey(mqtt_device_private_key());

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

  String topic;
  topic += "device/";
  topic += location_name();
  topic += "/data";
  clientMQTT.beginMessage(topic.c_str(), false, 1, 0);
  if (clientMQTT.write((uint8_t*)msg.c_str(), msg.length()) != msg.length()) {
    // I think this shouldn't happen since we checked above, but
    // it's not totally clear that it won't.  It's possible the
    // topic is also in the buffer, and there may be metadata too.
    log("Mqtt: Message was chopped!\n");
  }
  clientMQTT.endMessage();

#ifdef MQTT_VERBOSE
  log("Mqtt: Message sent!\n");
#endif

  delay(100);
  client.stop();
}

#endif
