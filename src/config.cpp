// SnappySense configuration manager
//
// larhan@knowit.no

#include "config.h"

// TODO: Most of these values should be factored into a config read from EEPROM or flash.
// One problem with that is that there's going to be quite a lot of data.

// --------- Access Point Credentials --------------------------------------------------
//
// Parameters for the WiFi network we're connecting to.
//
// The file ap_credentials.h is not in the repo.  DO NOT ADD IT.  This is a template:
/*
static const char *const WIFI_SSID = "........";
static const char *const WIFI_PASSWORD = ".............";
*/

#include "ap_credentials.h"

// --------- AWS Credentials -----------------------------------------------------------
//
// These are the AWS credentials we'll use to connect to the AWS MQTT broker.
//
// The file aws_credentials.h is not in the repo.  DO NOT ADD IT.  This is a template:
/*
static const char* AWS_IOT_ENDPOINT="...";
static const int AWS_MQTT_PORT=...;
static const char* AWS_CLIENT_IDENTIFIER="...";

// Amazon Root CA 1 (AmazonRootCA1.pem)
static const char AWS_CERT_CA[] = R"ROOTCA(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)ROOTCA";

// Device Certificate (XXXXXXXXXX-certificate.pem.crt)
static const char AWS_CERT_CRT[] PROGMEM = R"DEVICECERT(
-----BEGIN CERTIFICATE-----
...
-----END CERTIFICATE-----
)DEVICECERT";

// Device Private Key (XXXXXXXXXX-private.pem.key)
static const char AWS_CERT_PRIVATE[] PROGMEM = R"PRIVATEKEY(
-----BEGIN RSA PRIVATE KEY-----
...
-----END RSA PRIVATE KEY-----
)PRIVATEKEY";
*/

#ifdef MQTT_UPLOAD
#include "aws_credentials.h"
#endif

#if defined(WEB_UPLOAD) || defined(TIMESTAMP)
static const char* const SERVER_HOST = "raspberrypi.local";
static const int SERVER_PORT = 8086;
#endif

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

#if defined(WEB_UPLOAD) || defined(TIMESTAMP)
const char* server_host() {
  return SERVER_HOST;
}

int server_port() {
  return SERVER_PORT;
}
#endif

#ifdef WEB_UPLOAD
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
