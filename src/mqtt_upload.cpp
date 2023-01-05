// Code for uploading data to a remote MQTT broker.

#include "mqtt_upload.h"
#include "config.h"
#include "network.h"

// Can use a WiFiClientSecure to deal with certs, see 
// https://www.mischianti.org/2022/02/28/aws-iot-core-and-mqtt-services-connect-esp8266-devices-3/

#include <ArduinoMqttClient.h>
#include <WiFiClientSecure.h>

#ifdef MQTT_UPLOAD

void upload_results_to_mqtt_server(const SnappySenseData& data) {
#if 0
  // This is not great: we probably want a more flexible idea of whether the
  // wifi connection stays up or not.
  auto holder(connect_to_wifi());

  WiFiClientSecure client;
  const char* secret;
  secret = mqtt_root_ca_cert();
  client.setCACert(secret);
  secret = mqtt_device_cert();
  client.setCertificate(secret);
  secret = mqtt_device_private_key();
  client.setPrivateKey(secret);

  MqttClient clientMQTT(client);
#endif
}

#endif
