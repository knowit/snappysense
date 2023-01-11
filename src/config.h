// SnappySense configuration manager.  Anything that's a parameter that could sensibly
// be customized at runtime or stored in a configuration file is handled here.

#ifndef config_h_included
#define config_h_included

#include "main.h"

// Frequency of sensor readings.
//
// Note reading frequency is independent of both web and mqtt upload frequencies.
// Sensor readings draw little power and can be frequent, allowing for sampling
// if necessary.
unsigned long sensor_poll_frequency_seconds();

// The name of the location at which this device is placed.
const char* location_name();
void set_location_name(const char* name);

// WiFi access point SSID
const char* access_point_ssid(int n);

// WiFi access point password, this may be nullptr.
const char* access_point_password(int n);

bool device_enabled();
void set_device_enabled(bool flag);

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

// How often to upload results to a server.
//
// Note this is independent of mqtt upload, which is OK - web upload
// is for development and experimentation, mqtt upload for production.
unsigned long web_upload_frequency_seconds();
#endif

#ifdef MQTT_UPLOAD
// How often readings are captured and enqueued for mqtt upload.
//
// Note this is independent of sensor reading frequency; fewer readings
// may be captured for upload than are performed.
unsigned long mqtt_capture_frequency_seconds();
void set_mqtt_capture_frequency_seconds(unsigned long interval);

// How long will an idle connection (no outgoing or incoming messages) be
// kept alive?
unsigned long mqtt_max_idle_time_seconds();

// How long do we sleep between every time we bring up the radio for
// mqtt upload/download?
//
// Note this is independent of web upload, which is OK - web upload
// is for development and experimentation, mqtt upload for production.
unsigned long mqtt_sleep_interval_seconds();

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
// Note, STANDALONE will keep the device continually on.
unsigned long display_update_frequency_seconds();
#endif

#ifdef SERIAL_SERVER
// How long to wait between looking for input on the serial channel.
// This is typically a pretty low value.
//
// Note, SERIAL_SERVER will keep the device continually on.
unsigned long serial_command_poll_seconds();
#endif

#ifdef WEB_SERVER
// The port the device's server is listening on.
int web_server_listen_port();

// How long to wait between looking for input on the web channel.
// This is typically a pretty low value.
//
// Note, WEB_SERVER will keep the device continually on.
unsigned long web_command_poll_seconds();
#endif

#ifdef INTERACTIVE_CONFIGURATION
// See documentation in main.h
void interactive_configuration(Stream* io);
#endif

// Read configuration from some nonvolatile source, or revert to
// a default.
void read_configuration(Stream* io);

#endif // !config_h_included
