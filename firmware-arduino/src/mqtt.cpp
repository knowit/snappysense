// Code for uploading data to a remote MQTT broker and receiving messages we subscribe to.
//
// Summarized from the design document:
//
// The mqtt topic strings and JSON data formats are defined by MQTT-PROTOCOL.md.
//
// Summary:
//
// On startup, we publish to topic snappy/startup/<device-class>/<device-id> with
// fields "sent" (timestamp, string) and "interval" (observation interval,
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

#include "mqtt.h"

#ifdef SNAPPY_MQTT

#include <ArduinoMqttClient.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include "config.h"
#include "log.h"
#include "sensor.h"
#include "time_server.h"

// This version string identifies the snappy/startup/ JSON package and is sent as
// the "version" property of the package.
//
// The bugfix number is incremented if there's a compatible bugfix; for example, say we
// change how a floating point number is rounded.  This will very rarely be the case.
//
// The minor version is incremented if the package format is changed in a compatible
// manner, typically this will happen if we add or remove an optional field, or add
// a new non-optional field.
//
// The major version is incremented if the package format is changed in an incompatible
// way: the meaning of a field is changed in a nontrivial way, or a nonoptional field
// is removed, or the package format itself changes.
//
// Every field in the code for generate_startup_message() needs to be annotated with
// its version number.

#define STARTUP_VERSION "1.0.0"

// The default buffer size is 256 bytes on most devices.  That's too short for the
// sensor package, sometimes.  1K is OK - though may also be too short for some messages.
// We set the buffer size when we make the connection, but messages may arrive
// for sending after the connection has been set up.
//
// Also, I'm not sure yet how the length of the topic may fit into the buffer size
// calculation.
static const size_t MQTT_BUFFER_SIZE = 1024;

// The maximum number of messages to hold.  Each message can be, say, 0.5KB.  Once the
// queue fills up we discard the oldest messages.
static const size_t MAX_QUEUED = 100;

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
  CONNECTED,
  SUBSCRIBED,
  RUNNING,
  FAILED,
  STOPPED,
};

static MqttState mqtt_state;
static WiFiClient wifi_client;
static WiFiClientSecure wifi_client_secure;
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
#ifdef SNAPPY_TIMESTAMPS
static List<SnappySenseData> delayed_data_queue;
#endif

static void subscribe();
static void connect();
static bool poll();
static void send();
static void generate_startup_message();
static void mqtt_handle_message(int payload_size);
static void mqtt_enqueue(String&& topic, String&& body);
static void put_delayed_work();
static void enqueue_data(const SnappySenseData& data);

void mqtt_init() {
  mqtt_timer = xTimerCreate("mqtt", pdMS_TO_TICKS(500), pdFALSE, nullptr,
                            [](TimerHandle_t) { put_main_event(EvCode::COMM_MQTT_WORK); });
}

static bool should_send_delayed_data() {
#ifdef SNAPPY_TIMESTAMPS
  return !delayed_data_queue.is_empty() && time_adjustment() > 0;
#else
  return false;
#endif
}

#ifdef SNAPPY_TIMESTAMPS
static void drain_delayed_data() {
  time_t adj = time_adjustment();
  if (adj > 0) {
    while (!delayed_data_queue.is_empty()) {
      SnappySenseData d = delayed_data_queue.pop_front();
      d.time += adj;
      enqueue_data(d);
    }
  }
}
#endif

bool mqtt_have_work() {
  time_t delta = time(nullptr) - last_connect;

  // Hold data for a while, don't connect every time just because there's work to do.
  if ((!mqtt_queue.is_empty() && delta >= mqtt_upload_interval_s()) || should_send_delayed_data()) {
    return true;
  }

#ifdef SNAPPY_DEVELOPMENT
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
  // connect() moves the state machine into CONNECTING or CONNECTED (with a COMM_MQTT_WORK
  // message to drive the state machine) or FAILED (without a work message), work picks
  // up in mqtt_work().
  connect();
}

void mqtt_stop() {
  mqtt_client.stop();
  mqtt_state = MqttState::STOPPED;
}

void mqtt_work() {
  if (mqtt_state == MqttState::CONNECTING) {
    // This takes us back to CONNECTING, CONNECTED, or FAILED, see
    // comment in mqtt_start()
    connect();
    return;
  }
  if (mqtt_state == MqttState::CONNECTED) {
    if (!mqtt_client.connected()) {
      mqtt_stop();
      return;
    }
    subscribe();
    mqtt_state = MqttState::SUBSCRIBED;
    put_main_event(EvCode::COMM_ACTIVITY);
    put_main_event(EvCode::COMM_MQTT_WORK);
    return;
  }
  if (mqtt_state == MqttState::SUBSCRIBED) {
    if (!mqtt_client.connected()) {
      mqtt_stop();
      return;
    }
    mqtt_state = MqttState::RUNNING;
    if (send_startup_message) {
      generate_startup_message();
      send_startup_message = false;
    }
    put_main_event(EvCode::COMM_ACTIVITY);
    put_main_event(EvCode::COMM_MQTT_WORK);
    return;
  }
  if (mqtt_state == MqttState::RUNNING) {
    if (!mqtt_client.connected()) {
      mqtt_stop();
      return;
    }
#ifdef SNAPPY_TIMESTAMPS
    drain_delayed_data();
#endif
    if (!mqtt_queue.is_empty()) {
      send();
      put_main_event(EvCode::COMM_ACTIVITY);
    } else if (poll()) {
      // The normal case is that there's very little incoming traffic.  There may be
      // few actuators and the server should definitely limit the update frequency
      // for those.  There will be few instances of wishing to disable/enable devices
      // and changing their report frequencies.
      put_main_event(EvCode::COMM_ACTIVITY);
    }
    put_delayed_work();
    return;
  }
  /* STARTING, FAILED, STOPPED - ignore these for now */
}

static void enqueue_data(const SnappySenseData& data) {
  String topic;
  String body;

  // The topic string and JSON data format are defined by MQTT-PROTOCOL.md
  topic += "snappy/observation/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body = format_readings_as_json(data);

  mqtt_enqueue(std::move(topic), std::move(body));
}

void upload_add_data(SnappySenseData* data) {
  if (!device_enabled()) {
    delete data;
    return;
  }
  if (last_capture > 0 && time(nullptr) - last_capture < capture_interval_for_upload_s()) {
    delete data;
    return;
  }

  last_capture = time(nullptr);

#ifdef SNAPPY_TIMESTAMPS
  time_t adj = time_adjustment();
  if (adj == 0) {
    // Time has not been configured.  Need to enqueue the data for later.
    log("mqtt: holding message for later\n");
    SnappySenseData d = *data;
    delayed_data_queue.add_back(std::move(d));
    if (delayed_data_queue.length() > MAX_QUEUED) {
      delayed_data_queue.pop_front();
    }
    delete data;
    return;
  }
  // We have configured time.  Drain the queue before adding the new datum.
  drain_delayed_data();
#endif

  enqueue_data(*data);
  delete data;
}

static void mqtt_enqueue(String&& topic, String&& body) {
  mqtt_queue.add_back(std::move(MqttMessage(std::move(topic), std::move(body))));
  if (mqtt_queue.length() > MAX_QUEUED) {
    mqtt_queue.pop_front();
  }
}

static void put_delayed_work() {
  xTimerStart(mqtt_timer, portMAX_DELAY);
}

static void connect() {
again:
  switch (mqtt_state) {
    case MqttState::STARTING: {
      if (mqtt_tls()) {
        wifi_client_secure.setCACert(mqtt_root_ca_cert());
      }
      if (mqtt_auth_type() == MqttAuth::CERT_BASED && mqtt_device_cert() != nullptr && mqtt_device_private_key() != nullptr) {
        if (!mqtt_tls()) {
          panic("Secure client required for cert-based authentication\n");
        }
        wifi_client_secure.setCertificate(mqtt_device_cert());
        wifi_client_secure.setPrivateKey(mqtt_device_private_key());
      } else if (mqtt_auth_type() == MqttAuth::USER_AND_PASS && mqtt_username() != nullptr && mqtt_password() != nullptr) {
        // do nothing yet
      } else {
        log("Mqtt: Bad auth setting, possibly missing data?\n");
        mqtt_state = MqttState::FAILED;
        return;
      }

      if (mqtt_tls()) {
        mqtt_client.setClient(wifi_client_secure);
      } else {
        mqtt_client.setClient(wifi_client);
      }
      mqtt_client.setTxPayloadSize(MQTT_BUFFER_SIZE);
      mqtt_client.setCleanSession(false);

      mqtt_client.setId(mqtt_device_id());
      if (mqtt_auth_type() == MqttAuth::USER_AND_PASS) {
        mqtt_client.setUsernamePassword(mqtt_username(), mqtt_password());
      }

      log("Mqtt: Connecting to MQTT broker\n");
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
          put_delayed_work();
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
  if (*mqtt_device_id() != 0) {
    String command_msg("snappy/command/");
    command_msg += mqtt_device_id();
    mqtt_client.subscribe(command_msg, /* QoS= */ 1);
  }
}

static void generate_startup_message() {
  String topic;
  String body;

  // The topic string and JSON data format are defined by MQTT-PROTOCOL.md
  topic += "snappy/startup/";
  topic += mqtt_device_class();
  topic += "/";
  topic += mqtt_device_id();

  body += '{';

  // "version": mandatory, semver string, from version 1.0.0
  body += "\"version\":\"";
  body += STARTUP_VERSION;
  body += '"';

  // "sent": mandatory, unsigned number of seconds since Posix epoch, from version 1.0.0
  body += ",\"sent\":";
  body += format_timestamp(time(nullptr));

  // "interval": optional, unsigned number of seconds, from version 1.0.0
  body += ",\"interval\":";
  body += capture_interval_for_upload_s();

  body += '}';

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

  if (!mqtt_client.beginMessage(first.topic.c_str(), false, 1, 0)) {
    log("Mqtt: failed to setup connection\n");
    return;
  }
  if (mqtt_client.write((uint8_t*)first.message.c_str(), msg_len) != msg_len) {
    log("Mqtt: Message was chopped by mqtt layer!\n");
    // More than likely, the message will fail the next time too, so just discard it
    mqtt_queue.pop_front();
    return;
  }
  if (!mqtt_client.endMessage()) {
    log("Mqtt: Sending failed\n");
    return;
  }

  mqtt_queue.pop_front();
  log("Mqtt: Sent one datum\n");
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
    if (json.hasOwnProperty("version")) {
      // TODO: This should be checking the version, as that may determine the meaning or
      // presence of the rest of the fields.
      //
      // A missing version should be taken to equal 1.0.0, for compatibility reasons.
      //
      // Code below should be prepared to handle various versions, within reason.
      // By and large, the control message format should not change much, as every
      // new field should be optional, and the server should be able to send an
      // appropriate message for every device firmware version.  The device version
      // ought to be inferrable from the snappy/startup message.
    }
    if (json.hasOwnProperty("enable")) {
      // Boolean 0 or 1, whether to enable the device or not, from version 1.0.0
      unsigned flag = (unsigned)json["enable"];
      log("Mqtt: enable %u\n", flag);
      put_main_event(flag ? EvCode::ENABLE_DEVICE : EvCode::DISABLE_DEVICE);
      fields++;
    }
    if (json.hasOwnProperty("interval")) {
      // Unsigned number of seconds, sensor capture interval for upload, from version 1.0.0
      unsigned interval = (unsigned)json["interval"];
      log("Mqtt: set capture interval for upload %u\n", interval);
      put_main_event(EvCode::SET_INTERVAL, (uint32_t)interval);
      fields++;
    }
    // Don't send empty messages
    if (fields == 0) {
      log("Mqtt: invalid control message\n%s\n", buf);
    }
  } else if (topic.startsWith("snappy/command/")) {
    // No command messages defined at present
    log("Mqtt: invalid command message\n%s\n", buf);
  } else {
    log("Mqtt: unknown incoming message\n%s\n%s\n", topic.c_str(), buf);
  }
  work_done = true;
}

#endif
