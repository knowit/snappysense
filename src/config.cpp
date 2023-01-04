// SnappySense configuration manager

#include "config.h"

// Configuration parameters.
//
// The file client_config.h is not in the repo.  DO NOT ADD IT.  A template for the
// file is in client_config_template.txt.  In that file, search for FIXME.

#include "client_config.h"

static const char* const LOCATION_NAME = "lth";

#ifdef WEBSERVER
static const int WEB_SERVER_LISTEN_PORT = 8088;
#endif

#ifdef WEB_UPLOAD
static const unsigned long WEB_UPLOAD_WAIT_TIME_S = 60*5; // 5 minutes - OK for testing, gets us some volume
#endif

#ifdef MQTT_UPLOAD
static const unsigned long MQTT_UPLOAD_WAIT_TIME_S = 60*5; // 5 minutes - OK for testing, gets us some volume
#endif

#ifdef STANDALONE
static const unsigned long SCREEN_WAIT_TIME_S = 4;
#endif

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

int web_upload_frequency_seconds() {
  return WEB_UPLOAD_WAIT_TIME_S;
}
#endif

#ifdef MQTT_UPLOAD
int mqtt_upload_frequency_seconds() {
  return MQTT_UPLOAD_WAIT_TIME_S;
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
int display_update_frequency_seconds() {
  return SCREEN_WAIT_TIME_S;
}
#endif

const char* location_name() {
  return LOCATION_NAME;
}

#ifdef WEBSERVER
int web_server_listen_port() {
  return WEB_SERVER_LISTEN_PORT;
}
#endif