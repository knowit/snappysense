// SnappySense configuration manager

#include "config.h"

// Configuration parameters.
//
// The file client_config.h is not in the repo.  DO NOT ADD IT.  A template for the
// file is in client_config_template.txt.  In that file, search for FIXME.

#include "client_config.h"

static bool is_device_enabled = true;

#ifdef DEVELOPMENT
static const unsigned long SENSOR_POLL_FREQUENCY_S = 15;
#else
static const unsigned long SENSOR_POLL_FREQUENCY_S = 60;
#endif

#ifdef WEB_SERVER
static const int WEB_SERVER_LISTEN_PORT = 8088;
static const unsigned long WEB_SERVER_WAIT_TIME_S = 1;
#endif

#ifdef WEB_UPLOAD
// TODO: The upload frequency should ideally be a multiple of the sensor
// poll frequency; and/or there should be no upload if the sensor
// has not been read since the last time. 
#ifdef DEVELOPMENT
static const unsigned long WEB_UPLOAD_WAIT_TIME_S = 60*1;
#else
static const unsigned long WEB_UPLOAD_WAIT_TIME_S = 60*60; // 1 hour
#endif
#endif

#ifdef MQTT_UPLOAD
#ifdef DEVELOPMENT
static const unsigned long MQTT_UPLOAD_WAIT_TIME_S = 60*1;
static const unsigned long MQTT_SLEEP_INTERVAL_S = 60*2;
#else
static const unsigned long MQTT_UPLOAD_WAIT_TIME_S = 60*60; // 1 hour
static const unsigned long MQTT_SLEEP_INTERVAL_S = MQTT_UPLOAD_WAIT_TIME_S 
#endif
static const unsigned long MQTT_MAX_IDLE_TIME_S = 30;
static unsigned long mqtt_upload_wait_time_s = MQTT_UPLOAD_WAIT_TIME_S;
#endif

#ifdef STANDALONE
static const unsigned long SCREEN_WAIT_TIME_S = 4;
#endif

#ifdef SERIAL_SERVER
static const unsigned long SERIAL_SERVER_WAIT_TIME_S = 1;
#endif

bool device_enabled() {
  return is_device_enabled;
}

void set_device_enabled(bool flag) {
  is_device_enabled = flag;
}

unsigned long sensor_poll_frequency_seconds() {
  return SENSOR_POLL_FREQUENCY_S;
}

const char* location_name() {
  return LOCATION_NAME;
}

const char* access_point_ssid() {
    return WIFI_SSID;
}

const char* access_point_password() {
    return WIFI_PASSWORD;
}

#ifdef TIMESTAMP
const char* time_server_host() {
  return TIME_SERVER_HOST;
}

int time_server_port() {
  return TIME_SERVER_PORT;
}
#endif

#ifdef WEB_UPLOAD
const char* web_upload_host() {
  return WEB_UPLOAD_HOST;
}

int web_upload_port() {
  return WEB_UPLOAD_PORT;
}

unsigned long web_upload_frequency_seconds() {
  return WEB_UPLOAD_WAIT_TIME_S;
}
#endif

#ifdef MQTT_UPLOAD
unsigned long mqtt_capture_frequency_seconds() {
  return mqtt_upload_wait_time_s;
}

void set_mqtt_capture_frequency_seconds(unsigned long interval) {
  mqtt_upload_wait_time_s = interval;
}

unsigned long mqtt_max_idle_time_seconds() {
  return MQTT_MAX_IDLE_TIME_S;
}

unsigned long mqtt_sleep_interval_seconds() {
  return MQTT_SLEEP_INTERVAL_S;
}

const char* mqtt_endpoint_host() {
  return AWS_IOT_ENDPOINT;
}

int mqtt_endpoint_port() {
  return AWS_MQTT_PORT;
}

const char* mqtt_device_id() {
  return AWS_CLIENT_IDENTIFIER;
}

const char* mqtt_device_class() {
  return "snappysense";
}

const char* mqtt_root_ca_cert() {
  return AWS_CERT_CA;
}

const char* mqtt_device_cert() {
  return AWS_CERT_CRT;
}

const char* mqtt_device_private_key() {
  return AWS_CERT_PRIVATE;
}
#endif

#ifdef STANDALONE
unsigned long display_update_frequency_seconds() {
  return SCREEN_WAIT_TIME_S;
}
#endif

#ifdef WEB_SERVER
int web_server_listen_port() {
  return WEB_SERVER_LISTEN_PORT;
}

unsigned long web_command_poll_seconds() {
  return WEB_SERVER_WAIT_TIME_S;
}
#endif

#ifdef SERIAL_SERVER
unsigned long serial_command_poll_seconds() {
  return SERIAL_SERVER_WAIT_TIME_S;
}
#endif

