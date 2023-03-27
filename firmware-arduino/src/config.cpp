// SnappySense configuration manager

#include "config.h"
#include "device.h"
#include "log.h"
#include "util.h"

#include <Preferences.h>

// Configuration parameters.

#ifdef SNAPPY_DEVELOPMENT
// The file client_config.h provides compiled-in values so as to simplify development.
//
// Note, development_config.h is not in the repo, as it contains private values.  DO NOT ADD IT.
// A template for the file is in development_config_template.txt.  In that file, search for FIXME.
# include "development_config.h"
#endif

#ifdef SNAPPY_DEVELOPMENT
# define IF_DEVEL(x, y) x
#else
# define IF_DEVEL(x, y) y
#endif
#if defined(SNAPPY_DEVELOPMENT) && defined(SNAPPY_MQTT)
# define IF_MQTT_UP(x, y) x
#else
# define IF_MQTT_UP(x, y) y
#endif

// Each table of prefs has a last element whose long_key is nullptr.
//
// See comments further down about how these have evolved over time - additions, deletions, and name changes.
//
// Be sure not to duplicate short keys or to reuse short keys from deleted settings, old devices still
// have them in NVRAM.

static Pref factory_prefs[] = {
  {"enabled",                 "en",    Pref::Int,              1, "",                               "Device recording is enabled"},
  {"ssid1",                   "s1",    Pref::Str,              0, IF_DEVEL(WIFI_SSID, ""),          "SSID name for the first WiFi network"},
  {"ssid2",                   "s2",    Pref::Str,              0, "",                               "SSID name for the second WiFi network"},
  {"ssid3",                   "s3",    Pref::Str,              0, "",                               "SSID name for the third WiFi network"},
  {"password1",               "p1",    Pref::Str|Pref::Passwd, 0, IF_DEVEL(WIFI_PASSWORD, ""),      "Password for the first WiFi network"},
  {"password2",               "p2",    Pref::Str|Pref::Passwd, 0, "",                               "Password for the second WiFi network"},
  {"password3",               "p3",    Pref::Str|Pref::Passwd, 0, "",                               "Password for the third WiFi network"},
  {"web-config-access-point", "wcap",  Pref::Str,              0, "",                               "Unique access point name for end-user web config"},
  {"mqtt-use-tls",            "tls",   Pref::Int,              IF_MQTT_UP(MQTT_TLS, 0), "",         "MQTT TLS connection required (requires root cert)"},
  {"mqtt-auth",               "auth",  Pref::Str,              0, IF_MQTT_UP(MQTT_AUTH, ""),        "MQTT authorization method, \"pass\" or \"x509\""},
  {"mqtt-id",                 "aid",   Pref::Str,              0, IF_MQTT_UP(MQTT_ID, ""),          "MQTT device ID"},
  {"mqtt-class",              "acls",  Pref::Str,              0, IF_MQTT_UP("snappysense", ""),    "MQTT device class"},
  {"mqtt-endpoint-host",      "ahost", Pref::Str,              0, IF_MQTT_UP(MQTT_ENDPOINT, ""),    "MQTT endpoint host name"},
  {"mqtt-endpoint-port",      "aport", Pref::Int,              0, "",                               "MQTT port number"},
  {"mqtt-root-cert",          "aroot", Pref::Str|Pref::Cert,   0, IF_MQTT_UP(MQTT_ROOT_CERT, ""),   "MQTT root certificate (eg AmazonRootCA1.pem)"},
  {"mqtt-device-cert",        "acert", Pref::Str|Pref::Cert,   0, IF_MQTT_UP(MQTT_DEVICE_CERT, ""), "MQTT device certificate (eg XXXXXXXXXX-certificate.pem.crt)"},
  {"mqtt-private-key",        "akey",  Pref::Str|Pref::Cert,   0, IF_MQTT_UP(MQTT_DEVICE_KEY, ""),  "MQTT private key (eg XXXXXXXXXX-private.pem.key"},
  {"mqtt-username",           "unm",   Pref::Str,              0, IF_MQTT_UP(MQTT_USER, ""),        "MQTT username, for user/pass connection"},
  {"mqtt-password",           "pwd",   Pref::Str|Pref::Passwd, 0, IF_MQTT_UP(MQTT_PASS, ""),        "MQTT password, for user/pass connection"},
  { nullptr }
};

// `prefs` *must* be initialized with a call to reset_configuration() before its values
// are used for anything sensible.  The reason is that that operation copies in values
// from `factory_prefs`.

static Pref prefs[sizeof(factory_prefs)/sizeof(Pref)];

void reset_configuration() {
  Pref *fp = factory_prefs;
  Pref *p = prefs;
  while (fp->long_key != nullptr) {
    p->long_key = fp->long_key;
    p->short_key = fp->short_key;
    p->flags = fp->flags;
    p->str_value = fp->str_value;
    p->int_value = fp->int_value;
    p->help = fp->help;
    fp++;
    p++;
  }
}

Pref* get_pref(const char* name) {
  for (Pref* p = prefs; p->long_key != nullptr; p++) {
    if (strcmp(p->long_key, name) == 0) {
      return p;
    }
  }
  return nullptr;
}

int get_int_pref(const char* name) {
  return get_pref(name)->int_value;
}

static void set_int_pref(const char* name, int val) {
  get_pref(name)->int_value = val;
}

const char* get_string_pref(const char* name) {
  return get_pref(name)->str_value.c_str();
}

void set_string_pref(const char* name, const char* value) {
  get_pref(name)->str_value = value;
}

// Provisioning and run-time configuration.

void save_configuration() {
  // TODO: Issue 18: In principle the setting could fail due to fragmentation.
  // If so, we might be able to clear the prefs outright and then write
  // all of them again.
  Preferences nvr_prefs;
  if (nvr_prefs.begin("snappysense")) {
    for (Pref* p = prefs; p->long_key != nullptr; p++) {
      if (p->is_string()) {
        nvr_prefs.putString(p->short_key, p->str_value);
      } else {
        nvr_prefs.putInt(p->short_key, p->int_value);
      }
    }
    nvr_prefs.end();
  } else {
    log("Unable to open parameter store\n");
  }
}

void read_configuration() {
  reset_configuration();
  Preferences nvr_prefs;
  if (nvr_prefs.begin("snappysense", /* readOnly= */ true)) {
    log("Reading all prefs from parameter store\n");
    for (Pref* p = prefs; p->long_key != nullptr; p++) {
      if (!nvr_prefs.isKey(p->short_key)) {
        log("Not found: %s %s\n", p->long_key, p->short_key);
      } else if (p->is_string()) {
        p->str_value = nvr_prefs.getString(p->short_key);
      } else {
        p->int_value = nvr_prefs.getInt(p->short_key);
      }
    }
    nvr_prefs.end();
  } else {
    log("No configuration in parameter store\n");
  }
}

static String cert_first_line(const char* cert) {
  const char* p = strstr(cert, "BEGIN");
  const char* q = strchr(p, '\n');
  const char* r = strchr(q+1, '\n');
  return String((const uint8_t*)(q+1), (r-q-1));
}

void show_configuration(Stream* out) {
  for (Pref* p = prefs; p->long_key != nullptr; p++ ) {
    if (p->is_string()) {
      if (p->str_value.isEmpty()) {
        continue;
      }
      if (p->is_cert()) {
        out->printf("%-22s - %s...\n", p->long_key, cert_first_line(p->str_value.c_str()).c_str());
      } else if (p->is_passwd() && !p->str_value.isEmpty()) {
        out->printf("%-22s - %c.....\n", p->long_key, p->str_value[0]);
      } else {
        out->printf("%-22s - %s\n", p->long_key, p->str_value.c_str());
      }
    } else {
      out->printf("%-22s - %d\n", p->long_key, p->int_value);
    }
  }
}

// Configuration file format version, see CONFIG.md.   This is intended to be used in a
// proper "semantic versioning" way.
//
// Config version 1.1:
//   Introduced these new settings
//     web-config-access-point // short name "wcap" - name of snappysense access point
//
//   Made these identifiers no-ops
//     location              // short name "loc" - this was the location ID, now server-side only.
//     time-server-host      // short name "tsh" - this was a host name for the ad-hoc time server
//     time-server-port      // short name "tsp" -   and this was its port
//     http-upload-host      // short name "huh" - this was the host name for the ad-hoc http upload server
//     http-upload-port      // short name "hup" -   and this was its port
//
// Config version 2.0
//
//   Introduced these new settings
//     mqtt-use-tls          // short name "tls" - a flag, whether to require tls or not.  Required for mqtt-auth=="x509"
//     mqtt-auth             // short name "auth" - a string, "pass" or "x509"
//     mqtt-username         // short name "unm" - a string, the user name for "pass" authentication
//     mqtt-password         // short name "pwd" - a string, the password for "pass" authentication
//
//   Removed all support for these settings, which were obsoleted in v1.1
//     location              // short name "loc" - this was the location ID, now server-side only.
//     time-server-host      // short name "tsh" - this was a host name for the ad-hoc time server
//     time-server-port      // short name "tsp" -   and this was its port
//     http-upload-host      // short name "huh" - this was the host name for the ad-hoc http upload server
//     http-upload-port      // short name "hup" -   and this was its port
//
//   Changed the names of these settings
//     mqtt-endpoint-port    // short name "aport" - previously known as aws-iot-endpoint-port
//     mqtt-endpoint-host    // short name "ahost" - previously known as aws-iot-endpoint-host
//     mqtt-id               // short name "aid" - previously known as aws-iot-id
//     mqtt-class            // short name "acls" - previously known as aws-iot-class
//     mqtt-root-cert        // short name "aroot" - previously known as aws-iot-root-ca
//     mqtt-device-cert      // short name "acert" - previously known as aws-iot-device-cert, the device cert for "x509" authentication
//     mqtt-private-key      // short name "akey" - previously known as aws-iot-private-key, the private key for "x509" authentication

#define MAJOR_VERSION 2
#define MINOR_VERSION 0
#define BUGFIX_VERSION 0

// evaluate_config() evaluates a configuration program, using the `read_line` parameter
// to read lines of input from some source of text.
//
// If everything was fine it returns an empty String, and *was_saved is updated to indicate
// whether a save verb was present or not.
//
// On error, it returns a String with a longer error message, and also the line number and
// a short error message, suitable for the OLED screen.

String evaluate_configuration(List<String>& input, bool* was_saved, int* lineno, String* msg) {
  *lineno = 0;
  *was_saved = false;
  for (;;) {
    (*lineno)++;
    if (input.is_empty()) {
      *msg = "Missing END";
      return fmt("Line %d: Configuration program did not end with `end`", *lineno);
    }
    String line = input.pop_front();
    String kwd = get_word(line, 0);
    if (kwd == "end") {
      return String();
    } else if (kwd == "clear") {
      reset_configuration();
    } else if (kwd == "save") {
      save_configuration();
      log("Quit now and cake will be served immediately.\n");
      *was_saved = true;
    } else if (kwd == "version") {
      int major, minor, bugfix;
      if (sscanf(get_word(line, 1).c_str(), "%d.%d.%d", &major, &minor, &bugfix) != 3) {
        *msg = "Bad statement";
        return fmt("Line %d: Bad statement [%s]", *lineno, line.c_str());
      }
      if (major != MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION)) {
        // Ignore the bugfix version for now, but require it in the input
        *msg = "Bad version";
        return fmt("Line %d: Bad version %d.%d.%d, I'm %d.%d.%d",
                   *lineno,
                   major, minor, bugfix,
                   MAJOR_VERSION, MINOR_VERSION, BUGFIX_VERSION);
      }
      // version is OK
    } else if (kwd == "set") {
      String varname = get_word(line, 1);
      if (varname == "") {
        *msg = "Missing name";
        return fmt("Line %d: Missing variable name for 'set'", *lineno);
      }
      // It's legal to use "" as a value, but illegal not to have a value.
      bool flag = false;
      String value = get_word(line, 2, &flag);
      if (!flag) {
        *msg = "Missing value";
        return fmt("Line %d: Missing value for variable [%s]", *lineno, varname.c_str());
      }
      Pref* p = get_pref(varname.c_str());
      if (p == nullptr || p->is_cert()) {
        *msg = "Bad name";
        return fmt("Line %d: Unknown or inappropriate variable name for 'set': [%s]", *lineno, varname.c_str());
      }
      if (p->is_string()) {
        p->str_value = value;
      } else {
        p->int_value = int(value.toInt());
      }
    } else if (kwd == "cert") {
      String varname = get_word(line, 1);
      if (varname == "") {
        *msg = "Missing name";
        return fmt("Line %d: Missing variable name for 'cert'", *lineno);
      }
      String value;
      if (input.is_empty()) {
        *msg = "EOF in cert";
        return fmt("Line %d: Unexpected end of input in config (certificate)", *lineno);
      }
      (*lineno)++;
      String line = input.pop_front();
      if (!line.startsWith("-----BEGIN ")) {
        *msg = "Missing BEGIN";
        return fmt("Line %d: Expected -----BEGIN at the beginning of cert", *lineno);
      }
      value = line;
      value += "\n";
      for (;;) {
        if (input.is_empty()) {
          *msg = "EOF in cert";
          return fmt("Line %d: Unexpected end of input in config (certificate)", *lineno);
        }
        (*lineno)++;
        String line = input.pop_front();
        value += line;
        value += "\n";
        if (line.startsWith("-----END ")) {
          break;
        }
      }
      value.trim();
      Pref* p = get_pref(varname.c_str());
      if (p == nullptr || !p->is_cert()) {
        *msg = "Bad name";
        return fmt("Line %d: Unknown or inappropriate variable name for 'cert': [%s]", *lineno, varname.c_str());
      }
      p->str_value = value;
    } else {
      int i = 0;
      while (i < line.length() && isspace(line[i])) {
        i++;
      }
      if (i < line.length() && line[i] == '#') {
        // comment, do nothing
      } else if (i == line.length()) {
        // blank, do nothing
      } else {
        *msg = "Bad statement";
        return fmt("Line %d: Bad configuration statement [%s]", *lineno, line.c_str());
      }
    }
  }
  /*NOTREACHED*/
  panic("Unreachable state in evaluate_config");
}

// Non-configurable preferences.
//
// TODO: This is a bit of a mess; some are defined at the top and some are defined in-line.

#define MINUTE(s) ((s)*60)
#define HOUR(s) ((s)*60*60)

# if defined(SNAPPY_DEVELOPMENT)
static const unsigned long SLIDESHOW_CAPTURE_INTERVAL_S = MINUTE(1);
static const unsigned long MONITORING_CAPTURE_INTERVAL_S = MINUTE(1);
# else
static const unsigned long SLIDESHOW_CAPTURE_INTERVAL_S = MINUTE(1);
static const unsigned long MONITORING_CAPTURE_INTERVAL_S = MINUTE(30);
# endif

#ifdef SNAPPY_MQTT
# if defined(SNAPPY_DEVELOPMENT)
static const unsigned long MQTT_SLIDESHOW_UPLOAD_INTERVAL_S = MINUTE(2);
static const unsigned long MQTT_MONITORING_UPLOAD_INTERVAL_S = MINUTE(2);
# else
static const unsigned long MQTT_SLIDESHOW_UPLOAD_INTERVAL_S = MINUTE(5);
static const unsigned long MQTT_MONITORING_UPLOAD_INTERVAL_S = HOUR(4);
# endif
#endif

#ifdef SNAPPY_WIFI
// Comm window remains open 1min after last activity
unsigned long comm_activity_timeout_s() {
  return 60;
}

// Let the slideshow run 1min after comm window closes
unsigned long comm_relaxation_timeout_s() {
#ifdef SNAPPY_DEVELOPMENT
  return 30;
#else
  return 60;
#endif
}
#endif

// How long to wait in the sleep window in monitoring mode
unsigned long monitoring_mode_sleep_s() {
#ifdef SNAPPY_DEVELOPMENT
  return MINUTE(3);
#else
  return HOUR(1);
#endif
}
// How long to wait in the sleep window in slideshow mode
unsigned long slideshow_mode_sleep_s() {
#ifdef SNAPPY_DEVELOPMENT
  return MINUTE(1);
#else
  return MINUTE(5);
#endif
}

#ifdef SNAPPY_WIFI
unsigned long wifi_retry_ms() {
  return 500;
}
#endif

#ifdef SNAPPY_MQTT
unsigned long mqtt_upload_interval_s() {
  if (slideshow_mode) {
    return MQTT_SLIDESHOW_UPLOAD_INTERVAL_S;
  } else {
    return MQTT_MONITORING_UPLOAD_INTERVAL_S;
  }
}

unsigned long mqtt_max_unconnected_time_s() {
  return HOUR(4);
}
#endif

// The monitoring window *must* be longer than the value returned here.
unsigned long sensor_warmup_time_s() {
  if (slideshow_mode) {
    return 1;  // We're powered up, but zero is an invalid value
  } else {
    return 15; // Wild guess
  }
}

unsigned long monitoring_window_s() {
  // Constraints:
  // - longer than the warmup time
  // - long enough to get good readings from the PIR and MEMS after warmup
  return sensor_warmup_time_s() + 15;
}

#ifdef SNAPPY_NTP
unsigned long ntp_retry_s() {
  return 10;
}
#endif

unsigned long slideshow_update_interval_s() {
  return 2;
}

#ifdef SNAPPY_SERIAL_INPUT
unsigned long serial_server_poll_interval_ms() {
  return 1000;
}
#endif

// Preference accessors

unsigned long monitoring_capture_interval_s = MONITORING_CAPTURE_INTERVAL_S;

bool device_enabled() {
  return get_int_pref("enabled");
}

void set_device_enabled(bool flag) {
  set_int_pref("enabled", flag);
}

const char* access_point_ssid(int n) {
  switch (n) {
    case 1: return get_string_pref("ssid1");
    case 2: return get_string_pref("ssid2");
    case 3: return get_string_pref("ssid3");
    default: return "";
  }
}

void set_access_point_ssid(int n, const char* value) {
  switch (n) {
    case 1: set_string_pref("ssid1", value); break;
    case 2: set_string_pref("ssid2", value); break;
    case 3: set_string_pref("ssid3", value); break;
  }
}

const char* access_point_password(int n) {
  switch (n) {
    case 1: return get_string_pref("password1");
    case 2: return get_string_pref("password2");
    case 3: return get_string_pref("password3");
    default: return "";
  }
}

void set_access_point_password(int n, const char* value) {
  switch (n) {
    case 1: set_string_pref("password1", value); break;
    case 2: set_string_pref("password2", value); break;
    case 3: set_string_pref("password3", value); break;
  }
}

unsigned long capture_interval_s() {
  if (slideshow_mode) {
    return SLIDESHOW_CAPTURE_INTERVAL_S;
  } else {
    return monitoring_capture_interval_s;
  }
}

void set_capture_interval_s(unsigned long interval) {
  monitoring_capture_interval_s = interval;
}

#ifdef SNAPPY_MQTT
MqttAuth mqtt_auth_type() {
  const char* auth = get_string_pref("mqtt-auth");
  if (strcmp(auth, "pass") == 0) {
    if (mqtt_password() == nullptr || mqtt_username() == nullptr) {
      return MqttAuth::UNKNOWN;
    }
    return MqttAuth::USER_AND_PASS;
  }
  if (strcmp(auth, "x509") == 0) {
    if (!mqtt_tls() || mqtt_device_cert() == nullptr || mqtt_device_private_key() == nullptr) {
      return MqttAuth::UNKNOWN;
    }
    return MqttAuth::CERT_BASED;
  }
  return MqttAuth::UNKNOWN;
}

bool mqtt_tls() {
  return get_int_pref("mqtt-use-tls");
}

const char* mqtt_username() {
  return get_string_pref("mqtt-username");
}

const char* mqtt_password() {
  return get_string_pref("mqtt-password");
}

const char* mqtt_endpoint_host() {
  return get_string_pref("mqtt-endpoint-host");
}

int mqtt_endpoint_port() {
  int port = get_int_pref("mqtt-endpoint-port");
  if (port != 0) {
    return port;
  }
  if (mqtt_tls()) {
    return 8883;
  }
  return 1883;
}

const char* mqtt_device_id() {
  return get_string_pref("mqtt-id");
}

const char* mqtt_device_class() {
  return get_string_pref("mqtt-class");
}

const char* mqtt_root_ca_cert() {
  return get_string_pref("mqtt-root-cert");
}

const char* mqtt_device_cert() {
  return get_string_pref("mqtt-device-cert");
}

const char* mqtt_device_private_key() {
  return get_string_pref("mqtt-private-key");
}
#endif // SNAPPY_MQTT

#ifdef SNAPPY_WEBCONFIG
const char* web_config_access_point() {
  return get_string_pref("web-config-access-point");
}
#endif
