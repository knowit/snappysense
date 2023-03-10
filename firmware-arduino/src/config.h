// SnappySense configuration manager.
//
// Anything that's a parameter that could sensibly be customized at runtime or stored
// in a configuration file is handled here, and many compile-time parameters besides.

#ifndef config_h_included
#define config_h_included

#include "main.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////////
//
// Configuration management

// Reset the configuration (in memory) to "factory defaults", which is either almost nothing
// in release mode or the developer settings in SNAPPY_DEVELOPMENT mode.
void reset_configuration();

// Read configuration from some nonvolatile source, or revert to a default.
void read_configuration();

// Save current configuration in nvram.
void save_configuration();

// Dump the current configuration without revealing too many secrets.
void show_configuration(Stream* out);

// Evaluate the configuration script, returning the empty string on success and otherwise
// an error message.  `was_saved` is set to true if a `save` command was evaluated.
// `lineno` has the offending line number and `msg` a very short (OLED-suitable) error
// message in the case of error.
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

// Returns the pref if it is found, otherwise nullptr.
Pref* get_pref(const char* name);

/////////////////////////////////////////////////////////////////////////////////
//
// Device ID and status

// The device can be disabled and enabled by an mqtt message or during provisioning.
// If it is disabled is is still operable (responds to mqtt messages, for one thing)
// but does not read sensors or report their values.
bool device_enabled();

// Update the flag in RAM, but do not save the change in NVRAM.
void set_device_enabled(bool flag);

#ifdef SNAPPY_MQTT
// The device identifier required by the remote service
const char* mqtt_device_id();

// The name of the device class to which this device belongs
const char* mqtt_device_class();
#endif

/////////////////////////////////////////////////////////////////////////////////
//
// Sensors, monitoring, data capture

// How long does it take for sensors to warm up at the beginning of the monitoring window?
unsigned long sensor_warmup_time_s();

// How long is the monitoring window overall (including warmup)?
unsigned long monitoring_window_s();

#ifdef SNAPPY_MQTT
// How often readings are captured and enqueued for mqtt upload.
//
// Note this is independent of sensor reading frequency; fewer readings
// may be captured for upload than are performed.
unsigned long mqtt_capture_interval_s();
void set_mqtt_capture_interval_s(unsigned long interval);
#endif

/////////////////////////////////////////////////////////////////////////////////
//
// Networks

#ifdef SNAPPY_WIFI
// How long to wait between retries for connecting to an access point.
unsigned long wifi_retry_ms();

// How long to keep the wifi up after last recorded activity.
unsigned long comm_activity_timeout_s();
#endif

// WiFi access point SSID - up to three of them, n=1, 2, or 3.  If the ssid is
// not defined then the return value will be an empty string, not nullptr.
const char* access_point_ssid(int n);
void set_access_point_ssid(int n, const char* val);

// WiFi access point password.  Again n=1, 2, or 3.  If the password is empty
// then the return value is an empty string, not nullptr.
const char* access_point_password(int n);
void set_access_point_password(int n, const char* val);

#ifdef SNAPPY_NTP
// Interval between connection attempts to time server in the communication window.
unsigned long time_server_retry_s();
#endif

#ifdef SNAPPY_MQTT
// Host name and port to contact for MQTT traffic
const char* mqtt_endpoint_host();
int mqtt_endpoint_port();

enum class MqttAuth {
  UNKNOWN = 0,
  CERT_BASED = 1,
  USER_AND_PASS = 2
};
MqttAuth mqtt_auth_type();

// Use a Tls connection or not.  This can be configured as a setting, but the setting
// must be true for mqtt_auth_type() == CERT_BASED.  If a root cert is not defined then
// this must return false.  See the config consistency checks for more.
bool mqtt_tls();

// Keys and certificates
// Root cert for TLS connection
const char* mqtt_root_ca_cert();

// For user+pass-based connection (eg RabbitMQ testing setup): username and password
// These could be the same as device ID but that seems unnecessarily constraining
const char* mqtt_username();
const char* mqtt_password();

// For cert-based connection (eg AWS setup): device cert and device private key
const char* mqtt_device_cert();
const char* mqtt_device_private_key();
#endif

#ifdef SNAPPY_WEBCONFIG
// Name of the access point used for web configuration (AP mode).
const char* web_config_access_point();
#endif

/////////////////////////////////////////////////////////////////////////////////
//
// Power management, broadly

unsigned long slideshow_update_interval_s();

// How long to wait in the sleep window in monitoring mode
unsigned long monitoring_mode_sleep_s();

// How long to wait in the sleep window in slideshow mode
unsigned long slideshow_mode_sleep_s();

#ifdef SNAPPY_WIFI
// How long to wait after taking the wifi down before doing anything else.
unsigned long comm_relaxation_timeout_s();
#endif

#ifdef SNAPPY_SERIAL_INPUT
// How long to wait between polling for input on the serial channel. This is typically
// a pretty low value.
//
// Note, SNAPPY_SERIAL_INPUT services will keep the device continually on.
unsigned long serial_server_poll_interval_ms();
#endif

#ifdef SNAPPY_MQTT
// The longest time we will let pass without connecting, whether there are data
// to upload or not.
unsigned long mqtt_max_unconnected_time_s();

// Frequency of MQTT uploads, messages can be cached meanwhile.
unsigned long mqtt_upload_interval_s();
#endif

#endif // !config_h_included
