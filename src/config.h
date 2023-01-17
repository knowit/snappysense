// SnappySense configuration manager.  Anything that's a parameter that could sensibly
// be customized at runtime or stored in a configuration file is handled here, and
// many compile-time parameters besides.

#ifndef config_h_included
#define config_h_included

#include "main.h"
#include "microtask.h"
#include "serial_server.h"
#include "util.h"

// Frequency of sensor readings.
//
// Note reading frequency is independent of both web and mqtt upload frequencies.
// Sensor readings draw little power and can be frequent, allowing for sampling
// if necessary.
unsigned long sensor_poll_interval_s();

// The name of the location at which this device is placed.
const char* location_name();
void set_location_name(const char* name);

// WiFi access point SSID - up to three of them, n=1, 2, or 3.  If the ssid is
// not defined then the return value will be an empty string, not nullptr.
const char* access_point_ssid(int n);

// WiFi access point password.  Again n=1, 2, or 3.  If the password is empty
// then the return value is an empty string, not nullptr.
const char* access_point_password(int n);

// The device can be disabled and enabled by an mqtt message or during provisioning.
// If it is disabled is is still operable (responds to mqtt messages, for one thing)
// but does not read sensors or report their values.
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
unsigned long web_upload_interval_s();
#endif

#ifdef MQTT_UPLOAD
// How often readings are captured and enqueued for mqtt upload.
//
// Note this is independent of sensor reading frequency; fewer readings
// may be captured for upload than are performed.
unsigned long mqtt_capture_interval_s();
void set_mqtt_capture_interval_s(unsigned long interval);

// How long will an idle connection (no outgoing or incoming messages) be
// kept alive?
unsigned long mqtt_max_idle_time_s();

// How long do we sleep between every time we bring up the radio for
// mqtt upload/download?
//
// Note this is independent of web upload, which is OK - web upload
// is for development and experimentation, mqtt upload for production.
unsigned long mqtt_upload_interval_s();

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

#ifdef SLIDESHOW_MODE
// Note, SLIDESHOW_MODE will keep the device continually on.
unsigned long slideshow_update_interval_s();
#endif

#ifdef SNAPPY_SERIAL_LINE
// How long to wait between looking for input on the serial channel.
// This is typically a pretty low value.
//
// Note, SNAPPY_SERIAL_LINE services will keep the device continually on.
unsigned long serial_line_poll_interval_s();
#endif

#ifdef WEB_SERVER
// The port the device's server is listening on.
int web_server_listen_port();

// How long to wait between looking for input on the web channel.
// This is typically a pretty low value.
//
// Note, WEB_SERVER will keep the device continually on.
unsigned long web_command_poll_interval_s();
#endif

#ifdef INTERACTIVE_CONFIGURATION
class ReadSerialConfigInputTask final : public ReadSerialInputTask {
  enum {
    RUNNING,
    COLLECTING,
  } state = RUNNING;
  List<String> config_lines;
public:
  const char* name() override {
    return "Serial server config input";
  }
  void perform() override;
};
#endif

// Read configuration from some nonvolatile source, or revert to a default.
void read_configuration();

#endif // !config_h_included
