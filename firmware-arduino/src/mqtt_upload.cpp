// Code for uploading data to a remote MQTT broker and receiving messages we subscribe to.
//
// Summarized from the design document:
//
// The mqtt topic strings and JSON data formats are defined by aws-iot-backend/MQTT-PROTOCOL.md.
//
// Summary:
//
// On startup, we publish to topic snappy/startup/<device-class>/<device-id> with
// fields "time" (timestamp, string) and "interval" (observation interval,
// positive integer seconds).
//
// Following an observation, we publish to topic snappy/observation/<device-class>/<device-id> with
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

enum class MqttState {
  STARTING,
  CONNECTING,
  RETRYING,
  CONNECTED,
  SUBSCRIBED,
  RUNNING,
  FAILED,
  STOPPED,
};

static MqttState mqtt_state;
static WiFiClientSecure wifi_client;
static MqttClient mqtt_client(nullptr);
static int num_retries = 0;
static bool work_done;
static TimerHandle_t mqtt_timer;
static time_t last_connect;
static time_t last_capture;
static bool early_times = true;
static int num_times = 0;
static bool send_startup_message = true;
static List<MqttMessage> mqtt_queue;

static void subscribe();
static void connect();
static bool poll();
static void send();
static void generate_startup_message();
static void mqtt_handle_message(int payload_size);
static void mqtt_enqueue(String&& topic, String&& body);
static void put_delayed_retry();

void mqtt_init() {
  mqtt_timer = xTimerCreate("mqtt", pdMS_TO_TICKS(500), pdFALSE, nullptr,
                            [](TimerHandle_t) { put_main_event(EvCode::COMM_MQTT_WORK); });
}

bool mqtt_have_work() {
  time_t delta = time(nullptr) - last_connect;

  // Hold data for a while, don't connect every time just because there's work to do.
  if (!mqtt_queue.is_empty() && delta >= mqtt_upload_interval_s()) {
    return true;
  }

#ifdef DEVELOPMENT
  // For now, in development mode, upload always if there's stuff to upload.
  if (!mqtt_queue.is_empty()) {
    return true;
  }
#endif

  // We must connect every so often to check for commands, even if there's no outgoing
  // traffic.  This matters because the device can be disabled, in which case there will
  // be no outgoing traffic, but we depend on incoming traffic to enable it again.
  //
  // This calculation basically depends on time only having discontinuities forward, which
  // is reasonably safe for us.
  //
  // Early on we connect more often because commands are frequently delayed to later connections,
  // they are not always sent during the first comm window.
  if (delta >= mqtt_max_unconnected_time_s() || early_times) {
    return true;
  }
  return false;
}

void mqtt_start() {
  mqtt_state = MqttState::STARTING;
  num_retries = 0;
  if (early_times) {
    num_times++;
    if (num_times > 5) {
      early_times = false;
    }
  }
  connect();
}

void mqtt_stop() {
  mqtt_client.stop();
  mqtt_state = MqttState::STOPPED;
}

void mqtt_work() {
  if (mqtt_state == MqttState::CONNECTING) {
    connect();
    return;
  }
  if (mqtt_state == MqttState::CONNECTED) {
    subscribe();
    mqtt_state = MqttState::SUBSCRIBED;
    put_main_event(EvCode::COMM_ACTIVITY);
    put_main_event(EvCode::COMM_MQTT_WORK);
    return;
  }
  if (mqtt_state == MqttState::SUBSCRIBED) {
    mqtt_state = MqttState::RUNNING;
    if (send_startup_message) {
      generate_startup_message();
      put_main_event(EvCode::COMM_MQTT_WORK);
      send_startup_message = false;
      return;
    }
  }
  if (mqtt_state == MqttState::RUNNING) {
    if (!mqtt_queue.is_empty()) {
      send();
      put_main_event(EvCode::COMM_ACTIVITY);
      put_main_event(EvCode::COMM_MQTT_WORK); // to trigger the poll
      return;
    }
    // The normal case is that there's very little incoming traffic.  There will be
    // few actuators and the server should definitely limit the update frequency
    // for those.  There will be few instances of wishing to disable/enable devices
    // and changing their report frequencies.
    if (poll()) {
      put_main_event(EvCode::COMM_ACTIVITY);
    }
    put_delayed_retry();
  }
}

void mqtt_add_data(SnappySenseData* data) {
  if (!device_enabled()) {
    delete data;
    return;
  }
  if (last_capture > 0 && time(nullptr) - last_capture < mqtt_capture_interval_s()) {
    delete data;
    return;
  }

  last_capture = time(nullptr);

  String topic;
  String body;

  // The topic string and JSON data format are defined by aws-iot-backend/MQTT-PROTOCOL.md
  topic += "snappy/observation/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body = format_readings_as_json(*data);

  mqtt_enqueue(std::move(topic), std::move(body));
  delete data;
}

static void mqtt_enqueue(String&& topic, String&& body) {
  mqtt_queue.add_back(std::move(MqttMessage(std::move(topic), std::move(body))));
}

static void put_delayed_retry() {
  xTimerStart(mqtt_timer, portMAX_DELAY);
}

static void connect() {
again:
  switch (mqtt_state) {
    case MqttState::STARTING: {
      wifi_client.setCACert(mqtt_root_ca_cert());
      wifi_client.setCertificate(mqtt_device_cert());
      wifi_client.setPrivateKey(mqtt_device_private_key());

      mqtt_client.setClient(wifi_client);
      mqtt_client.setId(mqtt_device_id());
      mqtt_client.setTxPayloadSize(MQTT_BUFFER_SIZE);
      mqtt_client.setCleanSession(false);

      log("Mqtt: Connecting to AWS IOT\n");
      mqtt_state = MqttState::CONNECTING;
      goto again;
    }

    case MqttState::CONNECTING: {
      log("Mqtt: %s %d : %s\n", mqtt_endpoint_host(), mqtt_endpoint_port(), mqtt_device_id());
      bool ok = mqtt_client.connect(mqtt_endpoint_host(), mqtt_endpoint_port());
      put_main_event(EvCode::COMM_ACTIVITY);
      if (!ok) {
        // Positive error codes are basically fatal configuration errors and should
        // perhaps cause the mqtt component to be disabled.
        int res = mqtt_client.connectError();
        log("Mqtt: Failed %d\n", res);
        if (++num_retries < 10) {
          put_delayed_retry();
          return;
        }
        log("Mqtt: Rejected\n");
        mqtt_state = MqttState::FAILED;
        return;
      }
      log("Mqtt: Accepted\n");
      mqtt_state = MqttState::CONNECTED;
      last_connect = time(nullptr);
      put_main_event(EvCode::COMM_MQTT_WORK);
      return;
    }

    default:
      return;
  }
}

static void subscribe() {
  // Subscriptions used to be conditional on mqtt_first_time.  However,
  // at least for AWS and the Arduino MQTT stack, it seems like we have to
  // resubscribe every time, even if session is not marked as clean.
  if (*mqtt_device_id() != 0) {
    String control_msg("snappy/control/");
    control_msg += mqtt_device_id();
    mqtt_client.subscribe(control_msg, /* QoS= */ 1);
  }
  if (*mqtt_device_class() != 0) {
    String control_msg("snappy/control-class/");
    control_msg += mqtt_device_class();
    mqtt_client.subscribe(control_msg, /* QoS= */ 1);
  }
  String control_msg("snappy/control-all");
  mqtt_client.subscribe(control_msg, /* QoS= */ 1);
#ifdef MQTT_COMMAND_MESSAGES
  if (*mqtt_device_id() != 0) {
    String command_msg("snappy/command/");
    command_msg += mqtt_device_id();
    mqtt_client.subscribe(command_msg, /* QoS= */ 1);
  }
#endif
}

static void generate_startup_message() {
  String topic;
  String body;

  // The topic string and JSON data format are defined by aws-iot-backend/MQTT-PROTOCOL.md
  topic += "snappy/startup/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body += "{\"interval\":";
  body += mqtt_capture_interval_s();
  body += ",\"sent\":\"";
  body += format_timestamp(time(nullptr));
  body += "\"}";

  mqtt_enqueue(std::move(topic), std::move(body));
}

static bool poll() {
  work_done = false;
  mqtt_client.onMessage(mqtt_handle_message);
  mqtt_client.poll();
  mqtt_client.onMessage(nullptr);
  return work_done;
}

static void send() {
  if (mqtt_queue.is_empty()) {
    return;
  }

  MqttMessage& first = mqtt_queue.peek_front();
  size_t msg_len = first.message.length();
  if (msg_len > MQTT_BUFFER_SIZE) {
    log("Mqtt: Message too long: %d!\n", msg_len);
    mqtt_queue.pop_front();
    return;
  }

  // TODO: Status code!
  mqtt_client.beginMessage(first.topic.c_str(), false, 1, 0);
  // TODO: Status code!
  if (mqtt_client.write((uint8_t*)first.message.c_str(), msg_len) != msg_len) {
    log("Mqtt: Message was chopped by mqtt layer!\n");
  }
  // TODO: Status code!
  mqtt_client.endMessage();

  // FIXME: Issue 20: We could fail to send because the connection drops.  In that
  // case, detect the error and do not dequeue the message, but leave it in the buffer
  // for a subsequent attempt and exit the loop here.

  mqtt_queue.pop_front();
}

static void mqtt_handle_message(int payload_size) {
  static const size_t MAX_INCOMING_MESSAGE_SIZE = 1023;
  uint8_t buf[MAX_INCOMING_MESSAGE_SIZE+1];
  String topic = mqtt_client.messageTopic();
  if (payload_size > MAX_INCOMING_MESSAGE_SIZE) {
    log("Mqtt: Incoming message too long, %d bytes.  Message discarded.\n", payload_size);
    return;
  }
  int bytes_read = mqtt_client.read(buf, payload_size);
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
      put_main_event(flag ? EvCode::ENABLE_DEVICE : EvCode::DISABLE_DEVICE);
      fields++;
    }
    if (json.hasOwnProperty("interval")) {
      unsigned interval = (unsigned)json["interval"];
      log("Mqtt: set interval %u\n", interval);
      put_main_event(EvCode::SET_INTERVAL, (uint32_t)interval);
      fields++;
    }
    // Don't send empty messages
    if (fields == 0) {
      log("Mqtt: invalid control message\n%s\n", buf);
    }
#ifdef MQTT_COMMAND_MESSAGES
  } else if (topic.startsWith("snappy/command/")) {
    if (json.hasOwnProperty("actuator") &&
        json.hasOwnProperty("reading") &&
        json.hasOwnProperty("ideal")) {
      const char* key = (const char*)json["actuator"];
      double reading = (double)json["reading"];
      double ideal = (double)json["ideal"];
      log("Mqtt: actuate %s %f %f\n", key, reading, ideal);
      put_main_event(EvCode::ACTUATOR, new Actuator(reading, ideal));
    } else {
      log("Mqtt: invalid command message\n%s\n", buf);
    }
#endif
  } else {
    log("Mqtt: unknown incoming message\n%s\n%s\n", topic.c_str(), buf);
  }
  work_done = true;
}

#endif
