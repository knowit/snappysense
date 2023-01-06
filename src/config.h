// SnappySense configuration manager.  Anything that's a parameter that could sensibly
// be customized at runtime or stored in a configuration file is handled here.

#ifndef config_h_included
#define config_h_included

#include "main.h"

// Frequency of sensor readings.  This is independent of upload frequency.
unsigned long sensor_poll_frequency_seconds();

// The name of the location at which this device is placed.
const char* location_name();

// WiFi access point SSID
const char* access_point_ssid();

// WiFi access point password, this may be nullptr.
const char* access_point_password();

#ifdef TIMESTAMP
// Host name of remote web server used for web upload and time service.
const char* time_server_host();

// Port of remote web server used for web upload and time service.
int time_server_port();
#endif

#ifdef WEB_UPLOAD
// Host name of remote web server used for web upload and time service.
const char* web_upload_host();

// Port of remote web server used for web upload and time service.
int web_upload_port();

// How often to upload results to a server
unsigned long web_upload_frequency_seconds();
#endif

#ifdef MQTT_UPLOAD
// How often to upload results to a server
unsigned long mqtt_upload_frequency_seconds();

// Host name and port to contact for MQTT traffic
const char* mqtt_endpoint_host();
int mqtt_endpoint_port();

// The device identifier required by the remote service
const char* mqtt_device_id();

// The name of the device class to which this device belongs
const char* mqtt_device_class();

// Keys and certificates
const char* mqtt_root_ca_cert();
const char* mqtt_device_cert();
const char* mqtt_device_private_key();
#endif

#ifdef STANDALONE
unsigned long display_update_frequency_seconds();
#endif

#ifdef SERIAL_SERVER
// How long to wait between looking for input on the serial channel.
// This is typically a pretty low value.
unsigned long serial_command_poll_seconds();
#endif

#ifdef WEB_SERVER
// The port the device's server is listening on.
int web_server_listen_port();

// How long to wait between looking for input on the web channel.
// This is typically a pretty low value.
unsigned long web_command_poll_seconds();
#endif

#endif // !config_h_included
