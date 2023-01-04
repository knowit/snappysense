// SnappySense configuration manager.  Anything that's a parameter that could sensibly
// be customized at runtime or stored in a configuration file is handled here.
//
// larhan@knowit.no

#ifndef config_h_included
#define config_h_included

#include "main.h"

// WiFi access point SSID
const char* access_point_ssid();

// WiFi access point password, this may be nullptr.
const char* access_point_password();

#if defined(WEB_UPLOAD) || defined(TIMESTAMP)
// Host name of remote web server used for web upload and time service.
const char* server_host();

// Port of remote web server used for web upload and time service.
int server_port();
#endif

#ifdef WEB_UPLOAD
// How often to upload results to a server
int web_upload_frequency_seconds();
#endif

#ifdef MQTT_UPLOAD
// How often to upload results to a server
int mqtt_upload_frequency_seconds();

// Host name and port to contact for MQTT traffic
const char* mqtt_endpoint_host();
int mqtt_endpoint_port();

// The device identifier required by the remote service
const char* mqtt_device_id();

// Keys and certificates
const char* mqtt_root_ca_cert(size_t *size);
const char* mqtt_device_cert(size_t *size);
const char* mqtt_device_private_key(size_t *size);
#endif

#ifdef STANDALONE
int display_update_frequency_seconds();
#endif

// The name of the location at which this device is placed.
const char* location_name();

#ifdef WEBSERVER
// The port the device's server is listening on.
int web_server_listen_port();
#endif

#endif // !config_h_included
