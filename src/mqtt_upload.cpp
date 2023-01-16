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
// DESIGN: There might also be snappy/distress/<device-class>/<device-id> to report problems, or
// some ditto log message.  (In contrast, a ping should not be necessary because the mqtt broker
// ought to know when the device last connected.)
//
// We subscribe to two topics:
//
// A message published to snappy/control/<device-id> can have two fields, "enable" (0 or 1),
// and "interval" (mqtt capture interval, positive integer seconds).
//
// A message published to snappy/command/<device-id> has three fields, "actuator" (the environment
// factor we want to control, string, this should equal one of the json keys for the
// sensor object), "reading" (numeric, a value representing what the server thinks the
// current state of that factor is), and "ideal" (numeric, a value representing what
// the server thinks the factor should be).

#include "mqtt_upload.h"

#ifdef MQTT_UPLOAD

#include "config.h"
#include "log.h"
#include "microtask.h"
#include "network.h"
#include "snappytime.h"

#include <ArduinoMqttClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_Json.h>

// The default buffer size is 256 bytes on most devices.  That's too short for the
// sensor package, sometimes.  1K is OK - though may also be too short for some messages.
// We set the buffer size when we make the connection, but messages may arrive
// for sending after the connection has been set up.
//
// Also, I'm not sure yet how the length of the topic may fit into the buffer size
// calculation.
static const size_t MQTT_BUFFER_SIZE = 1024;

struct MqttMessage {
  MqttMessage(String&& topic, String&& message)
    : topic(std::move(topic)), message(std::move(message))
  {}
  String topic;
  String message;
};

static List<MqttMessage> mqtt_queue;

static struct MqttState {
  MqttState() : holder(false), mqtt(nullptr), work_done(false) {}
  WiFiHolder holder;
  WiFiClientSecure wifi;
  MqttClient mqtt;
  bool work_done;
} *mqtt_state;

static void mqtt_enqueue(String&& topic, String&& body) {
  mqtt_queue.add_back(std::move(MqttMessage(std::move(topic), std::move(body))));
}

void StartMqttTask::execute(SnappySenseData*) {
  String topic;
  String body;

  topic += "snappy/startup/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body += "{\"interval\":";
  body += sensor_poll_interval_s();
#ifdef TIMESTAMP
  body += ",\"time\":\"";
  body += format_time(snappy_local_time());
#endif
  body += "\"}";

  mqtt_enqueue(std::move(topic), std::move(body));
}

void CaptureSensorsForMqttTask::execute(SnappySenseData* data) {
  // FIXME: Issue 19: Number 0 is mostly bogus data so skip it
  if (data->sequence_number == 0) {
    return;
  }

  String topic;
  String body;

  topic += "snappy/reading/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body = format_readings_as_json(*data);

  mqtt_enqueue(std::move(topic), std::move(body));
}

// The MqttCommsTask will re-enqueue itself with the new deadline.
// FIXME: Issue 16: millis() is not a reliable API in the long term (beyond 49 days).

void MqttCommsTask::execute(SnappySenseData*) {
  unsigned long now = millis();
  // Note we don't connect just because there's something in the outgoing queue,
  // we wait until the upload is scheduled.  There could be a notion of high
  // priority messages that override this, but currently we don't need those.
  if (now < next_work) {
    sched_microtask_after(this, next_work - now);
    return;
  }

  // However, we do connect whether there is outgoing data or not, because we
  // want to poll for incoming messages.
  if (mqtt_state == nullptr) {
    mqtt_state = new MqttState();
    if (!connect()) {
      delete mqtt_state;
      mqtt_state = nullptr;
      return;
    }
  }
  if (!mqtt_queue.is_empty()) {
    send();
    // TODO: Undocumented embedded delay
    delay(100);
    last_work = millis();
  }
  // The normal case is that there's very little incoming traffic.  There will be
  // few actuators and the server should definitely limit the update frequency
  // for those.  There will be few instances of wishing to disable/enable devices
  // and changing their report frequencies.
  bool got_something = poll();
  if (got_something) {
    last_work = millis();
  }
  if (millis() - last_work >= mqtt_max_idle_time_s()*1000 || !mqtt_state->mqtt.connected()) {
    disconnect();
    delete mqtt_state;
    mqtt_state = nullptr;
    next_work = millis() + mqtt_upload_interval_s() * 1000;
  } else {
    next_work = millis() + 1000;
  }
  sched_microtask_after(this, max(next_work - millis(), 1000LU));
}

bool MqttCommsTask::connect() {
  mqtt_state->holder = connect_to_wifi();
  if (!mqtt_state->holder.is_valid()) {
    return false;
  }
  mqtt_state->wifi.setCACert(mqtt_root_ca_cert());
  mqtt_state->wifi.setCertificate(mqtt_device_cert());
  mqtt_state->wifi.setPrivateKey(mqtt_device_private_key());

  mqtt_state->mqtt.setClient(mqtt_state->wifi);
  mqtt_state->mqtt.setId(mqtt_device_id());
  mqtt_state->mqtt.setTxPayloadSize(MQTT_BUFFER_SIZE);

  mqtt_state->mqtt.setCleanSession(first_time);

  // Connect to the MQTT broker on the AWS endpoint
  // FIXME: Issue 15: don't block here
  log("Mqtt: Connecting to AWS IOT ");
  while (!mqtt_state->mqtt.connect(mqtt_endpoint_host(), mqtt_endpoint_port())) {
    log(".");
    // TODO: Embedded delay
    delay(100);
  }
  if(!mqtt_state->mqtt.connected()){
    log("AWS IoT Timeout!\n");
    return false;
  }
  log("Connected!\n");

  // Subscriptions used to be conditional on mqtt_first_time.  However,
  // at least for AWS and the Arduino MQTT stack, it seems like we have to
  // resubscribe every time, even if session is not marked as clean.
  String control_msg("snappy/control/");
  control_msg += mqtt_device_id();
  mqtt_state->mqtt.subscribe(control_msg, /* QoS= */ 1);
  String command_msg("snappy/command/");
  command_msg += mqtt_device_id();
  mqtt_state->mqtt.subscribe(command_msg, /* QoS= */ 1);
  first_time = false;
  return true;
}

void MqttCommsTask::disconnect() {
  mqtt_state->mqtt.stop();
  mqtt_state->holder = WiFiHolder(false);
}

void MqttCommsTask::send() {
  log("A\n");
  while (!mqtt_queue.is_empty()) {
    log("B\n");
    MqttMessage& first = mqtt_queue.peek_front();
    log("C\n");
    size_t msg_len = first.message.length();
    log("D\n");
    if (msg_len > MQTT_BUFFER_SIZE) {
      log("Mqtt: Message too long: %d!\n", msg_len);
      continue;
    }
    log("E\n");

    mqtt_state->mqtt.beginMessage(first.topic.c_str(), false, 1, 0);
    log("F\n");
    if (mqtt_state->mqtt.write((uint8_t*)first.message.c_str(), msg_len) != msg_len) {
      log("Mqtt: Message was chopped by mqtt layer!\n");
    }
    log("G\n");
    mqtt_state->mqtt.endMessage();
    log("H\n");

    // FIXME: Issue 20: We could fail to send because the connection drops.  In that
    // case, detect the error and do not dequeue the message, but leave it in the buffer
    // for a subsequent attempt and exit the loop here.

    mqtt_queue.pop_front();
    log("I\n");
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

  // Technically we should check that there are no unknown fields here.
  // Not sure if we care that much.
  JSONVar json = JSON.parse((const char*)buf);
  if (topic.startsWith("snappy/control/")) {
    int fields = 0;
    if (json.hasOwnProperty("enable")) {
      unsigned flag = (unsigned)json["enable"];
      log("Mqtt: enable %u\n", flag);
      sched_microtask_after(new EnableDeviceTask(!!flag), 0);
      fields++;
    }
    if (json.hasOwnProperty("interval")) {
      unsigned interval = (unsigned)json["interval"];
      log("Mqtt: set interval %u\n", interval);
      sched_microtask_after(new SetMqttIntervalTask(interval), 0);
      fields++;
    }
    // Don't send empty messages
    if (fields == 0) {
      log("Mqtt: invalid control message\n%s\n", buf);
    }
  } else if (topic.startsWith("snappy/command/")) {
    if (json.hasOwnProperty("actuator") &&
        json.hasOwnProperty("reading") && 
        json.hasOwnProperty("ideal")) {
      const char* key = (const char*)json["actuator"];
      double reading = (double)json["reading"];
      double ideal = (double)json["ideal"];
      log("Mqtt: actuate %s %f %f\n", key, reading, ideal);
      sched_microtask_after(new RunActuatorTask(String(key), reading, ideal), 0);
    } else {
      log("Mqtt: invalid command message\n%s\n", buf);      
    }
  } else {
    log("Mqtt: unknown incoming message\n%s\n%s\n", topic.c_str(), buf);
  }
  mqtt_state->work_done = true;
}

bool MqttCommsTask::poll() {
  mqtt_state->work_done = false;
  mqtt_state->mqtt.onMessage(mqtt_handle_message);
  mqtt_state->mqtt.poll();
  mqtt_state->mqtt.onMessage(nullptr);
  return mqtt_state->work_done;
}

void SetMqttIntervalTask::execute(SnappySenseData*) {
  set_mqtt_capture_interval_s(interval_s);
}
#endif
