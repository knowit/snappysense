// SnappySense configuration manager.  Anything that's a parameter that could sensibly
// be customized at runtime or stored in a configuration file is handled here, and
// many compile-time parameters besides.

#ifndef config_h_included
#define config_h_included

#include "main.h"
#include "serial_input.h"
#include "util.h"

// Frequency of sensor readings.
//
// Note reading frequency is independent of both web and mqtt upload frequencies.
// Sensor readings draw little power and can be frequent, allowing for sampling
// if necessary.
unsigned long sensor_poll_interval_s();

// How long does it take for sensors to warm up at the beginning of the monitoring window?
unsigned long sensor_warmup_time_s();

// The name of the location at which this device is placed.
const char* location_name();
void set_location_name(const char* name);

// WiFi access point SSID - up to three of them, n=1, 2, or 3.  If the ssid is
// not defined then the return value will be an empty string, not nullptr.
const char* access_point_ssid(int n);
void set_access_point_ssid(int n, const char* val);

// WiFi access point password.  Again n=1, 2, or 3.  If the password is empty
// then the return value is an empty string, not nullptr.
const char* access_point_password(int n);
void set_access_point_password(int n, const char* val);

// The device can be disabled and enabled by an mqtt message or during provisioning.
// If it is disabled is is still operable (responds to mqtt messages, for one thing)
// but does not read sensors or report their values.
bool device_enabled();
void set_device_enabled(bool flag);

#ifdef TIMESERVER
// Host name of remote web server used for web upload and time service.
const char* time_server_host();

// Port of remote web server used for web upload and time service.
int time_server_port();

// Interval between connection attempts to time server in the communication window.
unsigned long time_server_retry_s();
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

unsigned long slideshow_update_interval_s();

// How long to wait in the sleep window in monitoring mode
unsigned long monitoring_mode_sleep_s();

// How long to wait in the sleep window in slideshow mode
unsigned long slideshow_mode_sleep_s();

#ifdef SNAPPY_SERIAL_INPUT
// How long to wait between looking for input on the serial channel.
// This is typically a pretty low value.
//
// Note, SNAPPY_SERIAL_LINE services will keep the device continually on.
unsigned long serial_input_poll_interval_s();
#endif

#ifdef WEB_CONFIGURATION
const char* web_config_access_point();
#endif

#ifdef SNAPPY_WIFI
unsigned long wifi_retry_ms();
unsigned long comm_activity_timeout_s();
unsigned long comm_relaxation_timeout_s();
#endif

// Reset the configuration (in memory) to "factory defaults", which is either almost nothing
// in release mode or the developer settings in DEVELOPER mode.
void reset_configuration();

// Read configuration from some nonvolatile source, or revert to a default.
void read_configuration();

// Save current configuration in nvram.
void save_configuration();

// Dump the current configuration without revealing too many secrets.
void show_configuration(Stream* out);

// Evaluate the configuration script, returning the empty string on success and otherwise
// an error message.
String evaluate_configuration(List<String>& input, bool* was_saved, int* lineno, String* msg);

// A structure holding a preference value.
//
// By and large no strings will have leading or trailing whitespace unless they were
// defined by quoted strings that include such whitespace.

struct Pref {
  enum Flags {
    Str = 1,     // str_value has value
    Int = 2,     // int_value has value
    Cert = 4,    // Must also be Str: is certificate, not simple value
    Passwd = 8   // Must also be Str: is password
  };
  const char* long_key;   // The key name used in the config script
  const char* short_key;  // The key name used in NVRAM
  int flags;              // Bitwise 'or' of flags above
  int int_value;          // Valid iff flags & Int
  String str_value;       // Valid iff flags & Str
  const char* help;       // Arbitrary text

  bool is_string() { return flags & Str; }
  bool is_int() { return flags & Int; }
  bool is_cert() { return flags & Cert; }
  bool is_passwd() { return flags & Passwd; }
};

// A global table of current preferences.  The last entry is a sentinel with
// long_key==nullptr.

extern Pref prefs[];

// Returns the pref if it is found, otherwise nullptr.

Pref* get_pref(const char* name);

#endif // !config_h_included
