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
// A message published to snappy/control/<device-id> can have two fields, "enable" (0 or 1),
// and "interval" (reading interval, positive integer seconds).
//
// A message published to snappy/command/<device-id> has three fields, "actuator" (the environment
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

// The default buffer size is 256 bytes on most devices.  That's too short for the
// sensor package, sometimes.  1K is OK - though may also be too short for some messages.
// We set the buffer size when we make the connection, but messages may arrive
// for sending after the connection has been set up.
//
// Also, I'm not sure yet how the length of the topic may fit into the buffer size
// calculation.
static const size_t MQTT_BUFFER_SIZE = 1024;

static bool mqtt_first_time = true;
static unsigned long mqtt_next_work;  // millisecond timestamp
static unsigned long mqtt_last_work;  // millisecond timestamp

static void mqtt_connect();
static void mqtt_disconnect();
static void mqtt_send();
static bool mqtt_poll();

static struct MqttMessage {
  MqttMessage(String&& topic, String&& message)
    : topic(std::move(topic)), message(std::move(message)), next(nullptr)
  {}
  String topic;
  String message;
  MqttMessage* next;
} *mqtt_queue, *mqtt_last;

static struct MqttState {
  MqttState() : holder(false), mqtt(nullptr), work_done(false) {}
  WiFiHolder holder;
  WiFiClientSecure wifi;
  MqttClient mqtt;
  bool work_done;
} *mqtt_state;

static void mqtt_enqueue(String&& topic, String&& body) {
  auto* p = new MqttMessage(std::move(topic), std::move(body));
  if (mqtt_queue == nullptr) {
    mqtt_queue = mqtt_last = p;
  } else {
    mqtt_last->next = p;
    mqtt_last = p;
  }
}

void start_mqtt() {
  String topic;
  String body;

  topic += "snappy/startup/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body += "{\"interval\":";
  body += sensor_poll_frequency_seconds();
#ifdef TIMESTAMP
  body += ",\"time\":\"";
  body += format_time(snappy_local_time());
#endif
  body += "\"}";

  mqtt_enqueue(std::move(topic), std::move(body));
}

void capture_readings_for_mqtt_upload(const SnappySenseData& data) {
  // Number 0 is mostly bogus data so skip it
  if (data.sequence_number == 0) {
    return;
  }

  String topic;
  String body;

  topic += "snappy/reading/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body = format_readings_as_json(data);

  mqtt_enqueue(std::move(topic), std::move(body));
}

// This returns a delay in seconds until the next time it's interesting to
// call it again.  TODO: See comments in main.cpp about the return value.

unsigned long perform_mqtt_step() {
  unsigned long now = millis();
  // Note we don't connect just because there's something in the outgoing queue,
  // we wait until the upload is scheduled.  There could be a notion of high
  // priority messages that override this, but currently we don't need those.
  if (now < mqtt_next_work) {
    return (mqtt_next_work - now)/1000;
  }
  // However, we do connect whether there is outgoing data or not, because we
  // want to poll for incoming messages.
  if (mqtt_state == nullptr) {
    mqtt_state = new MqttState();
    mqtt_connect();
  }
  if (mqtt_queue != nullptr) {
    mqtt_send();
    delay(100);
    mqtt_last_work = millis();
  }
  // The normal case is that there's very little incoming traffic.  There will be
  // few actuators and the server should definitely limit the update frequency
  // for those.  There will be few instances of wishing to disable/enable devices
  // and changing their report frequencies.
  bool got_something = mqtt_poll();
  if (got_something) {
    mqtt_last_work = millis();
  }
  if (millis() - mqtt_last_work >= mqtt_max_idle_time_seconds()*1000 || !mqtt_state->mqtt.connected()) {
    mqtt_disconnect();
    delete mqtt_state;
    mqtt_state = nullptr;
    mqtt_next_work = millis() + mqtt_sleep_interval_seconds() * 1000;
  } else {
    mqtt_next_work = millis() + 1000;
  }
  return max((mqtt_next_work - millis())/1000, 1LU);
}

static void mqtt_connect() {
  mqtt_state->holder = connect_to_wifi();
  mqtt_state->wifi.setCACert(mqtt_root_ca_cert());
  mqtt_state->wifi.setCertificate(mqtt_device_cert());
  mqtt_state->wifi.setPrivateKey(mqtt_device_private_key());

  mqtt_state->mqtt.setClient(mqtt_state->wifi);
  mqtt_state->mqtt.setId(mqtt_device_id());
  mqtt_state->mqtt.setTxPayloadSize(MQTT_BUFFER_SIZE);

  mqtt_state->mqtt.setCleanSession(mqtt_first_time);

  // Connect to the MQTT broker on the AWS endpoint
  log("Mqtt: Connecting to AWS IOT ");
  while (!mqtt_state->mqtt.connect(mqtt_endpoint_host(), mqtt_endpoint_port())) {
    log(".");
    delay(100);
  }
  if(!mqtt_state->mqtt.connected()){
    log("AWS IoT Timeout!\n");
    return;
  }
  log("Connected!\n");

  if (mqtt_first_time) {
    String control_msg("snappy/control/");
    control_msg += mqtt_device_id();
    mqtt_state->mqtt.subscribe(control_msg, /* QoS= */ 1);
    String command_msg("snappy/command/");
    command_msg += mqtt_device_id();
    mqtt_state->mqtt.subscribe(command_msg, /* QoS= */ 1);
    mqtt_first_time = false;
  }
}

static void mqtt_disconnect() {
  mqtt_state->mqtt.stop();
  mqtt_state->holder = WiFiHolder(false);
}

static void mqtt_send() {
  while (mqtt_queue != nullptr) {
    size_t msg_len = mqtt_queue->message.length();
    if (msg_len > MQTT_BUFFER_SIZE) {
      log("Mqtt: Message too long!\n");
      continue;
    }

    mqtt_state->mqtt.beginMessage(mqtt_queue->topic.c_str(), false, 1, 0);
    if (mqtt_state->mqtt.write((uint8_t*)mqtt_queue->message.c_str(), msg_len) != msg_len) {
      log("Mqtt: Message was chopped by mqtt layer!\n");
    }
    mqtt_state->mqtt.endMessage();

    // TODO: In principle we could fail to send because the connection drops.  In that
    // case, detect the error and do not dequeue the message, but leave it in the buffer
    // for a subsequent attempt and exit the loop here.

    auto* it = mqtt_queue;
    mqtt_queue = it->next;
    delete it;
  }
  if (mqtt_queue == nullptr) {
    mqtt_last = nullptr;
  }
}

static void mqtt_handle_message(int payload_size) {
  static const size_t MAX_INCOMING_MESSAGE_SIZE = 1023;
  uint8_t buf[MAX_INCOMING_MESSAGE_SIZE+1];
  String topic = mqtt_state->mqtt.messageTopic();
  if (payload_size > MAX_INCOMING_MESSAGE_SIZE) {
    log("Mqtt: Incoming message too long, %d bytes.  Message discarded.\n", payload_size);
    return;
  }
  int bytes_read = mqtt_state->mqtt.read(buf, payload_size);
  if (bytes_read != payload_size) {
    log("Mqtt: Bytes read not equal to bytes expected, %d %d.  Message discarded.\n", bytes_read, payload_size);
    return;
  }
  buf[payload_size] = 0;
  log("Mqtt: incoming message\n%s\n%s\n", topic.c_str(), buf);
  mqtt_state->work_done = true;
}

static bool mqtt_poll() {
  mqtt_state->work_done = false;
  mqtt_state->mqtt.onMessage(mqtt_handle_message);
  mqtt_state->mqtt.poll();
  mqtt_state->mqtt.onMessage(nullptr);
  return mqtt_state->work_done;
}

#endif
