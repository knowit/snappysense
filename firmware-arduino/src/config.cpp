// SnappySense configuration manager

#include "config.h"
#include "device.h"
#include "log.h"
#include "util.h"

#include <Preferences.h>

// Configuration parameters.

#ifdef DEVELOPMENT
// The file client_config.h provides compiled-in values so as to simplify development.
//
// Note, development_config.h is not in the repo, as it contains private values.  DO NOT ADD IT.
// A template for the file is in development_config_template.txt.  In that file, search for FIXME.
# include "development_config.h"
#endif

#ifdef DEVELOPMENT
# define IF_DEVEL(x, y) x
#else
# define IF_DEVEL(x, y) y
#endif
#if defined(DEVELOPMENT) && defined(TIMESERVER)
# define IF_TIMESERVER(x, y) x
#else
# define IF_TIMESERVER(x, y) y
#endif
#if defined(DEVELOPMENT) && defined(MQTT_UPLOAD)
# define IF_MQTT_UP(x, y) x
#else
# define IF_MQTT_UP(x, y) y
#endif

// Each table of prefs has a last element whose long_key is nullptr.

static Pref factory_prefs[] = {
  {"enabled",                 "en",    Pref::Int,              1, "",                                    "Device recording is enabled"},
  {"location",                "loc",   Pref::Str,              0, IF_DEVEL(LOCATION_NAME, ""),           "Name of device location"},
  {"ssid1",                   "s1",    Pref::Str,              0, IF_DEVEL(WIFI_SSID, ""),               "SSID name for the first WiFi network"},
  {"ssid2",                   "s2",    Pref::Str,              0, "",                                    "SSID name for the second WiFi network"},
  {"ssid3",                   "s3",    Pref::Str,              0, "",                                    "SSID name for the third WiFi network"},
  {"password1",               "p1",    Pref::Str|Pref::Passwd, 0, IF_DEVEL(WIFI_PASSWORD, ""),           "Password for the first WiFi network"},
  {"password2",               "p2",    Pref::Str|Pref::Passwd, 0, "",                                    "Password for the second WiFi network"},
  {"password3",               "p3",    Pref::Str|Pref::Passwd, 0, "",                                    "Password for the third WiFi network"},
  {"time-server-host",        "tsh",   Pref::Str,              0, IF_TIMESERVER(TIME_SERVER_HOST, ""),   "Host name of ad-hoc time server"},
  {"time-server-port",        "tsp",   Pref::Int,              IF_TIMESERVER(TIME_SERVER_PORT, 8086), "","Port name on the ad-hoc time server"},
  {"http-upload-host",        "huh",   Pref::Str,              0, "",                                    "Host name of ad-hoc http sensor-reading upload server"},
  {"http-upload-port",        "hup",   Pref::Int,              8086, "",                                 "Port number on the ad-hoc http sensor-reading upload server"},
  {"web-config-access-point", "wcap",  Pref::Str,              0, "",                                    "Unique access point name for end-user web config"},
  {"aws-iot-id",              "aid",   Pref::Str,              0, IF_MQTT_UP(AWS_CLIENT_IDENTIFIER, ""), "IoT device ID"},
  {"aws-iot-class",           "acls",  Pref::Str,              0, IF_MQTT_UP("snappysense", ""),         "IoT device class"},
  {"aws-iot-endpoint-host",   "ahost", Pref::Str,              0, IF_MQTT_UP(AWS_IOT_ENDPOINT, ""),      "IoT endpoint host name"},
  {"aws-iot-endpoint-port",   "aport", Pref::Int,              IF_MQTT_UP(AWS_MQTT_PORT, 8883), "",      "IoT port number"},
  {"aws-iot-root-ca",         "aroot", Pref::Str|Pref::Cert,   0, IF_MQTT_UP(AWS_CERT_CA, ""),           "Root CA certificate (AmazonRootCA1.pem)"},
  {"aws-iot-device-cert",     "acert", Pref::Str|Pref::Cert,   0, IF_MQTT_UP(AWS_CERT_CRT, ""),          "Device certificate (XXXXXXXXXX-certificate.pem.crt)"},
  {"aws-iot-private-key",     "akey",  Pref::Str|Pref::Cert,   0, IF_MQTT_UP(AWS_CERT_PRIVATE, ""),      "Private key (XXXXXXXXXX-private.pem.key"},
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
// Version 1.1:
//   Added web-config-access-point

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
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

#ifdef MQTT_UPLOAD
# if defined(DEVELOPMENT)
static const unsigned long MQTT_SLIDESHOW_CAPTURE_INTERVAL_S = MINUTE(1);
static const unsigned long MQTT_MONITORING_CAPTURE_INTERVAL_S = MINUTE(1);
static const unsigned long MQTT_SLIDESHOW_UPLOAD_INTERVAL_S = MINUTE(2);
static const unsigned long MQTT_MONITORING_UPLOAD_INTERVAL_S = MINUTE(2);
# else
#  ifdef TEST_POWER_MANAGEMENT
static const unsigned long MQTT_SLIDESHOW_CAPTURE_INTERVAL_S = MINUTE(5);
static const unsigned long MQTT_MONITORING_CAPTURE_INTERVAL_S = MINUTE(5);
#  else
static const unsigned long MQTT_SLIDESHOW_CAPTURE_INTERVAL_S = MINUTE(1);
static const unsigned long MQTT_MONITORING_CAPTURE_INTERVAL_S = MINUTE(30);
#  endif
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
#ifdef DEVELOPMENT
  return 30;
#else
  return 60;
#endif
}
#endif

// How long to wait in the sleep window in monitoring mode
unsigned long monitoring_mode_sleep_s() {
#ifdef DEVELOPMENT
  return MINUTE(3);
#else
  return HOUR(1);
#endif
}
// How long to wait in the sleep window in slideshow mode
unsigned long slideshow_mode_sleep_s() {
#ifdef DEVELOPMENT
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

#ifdef MQTT_UPLOAD
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
  return 30;
}

#ifdef TIMESERVER
unsigned long time_server_retry_s() {
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

#ifdef MQTT_UPLOAD
unsigned long mqtt_monitoring_capture_interval_s = MQTT_MONITORING_CAPTURE_INTERVAL_S;
#endif

bool device_enabled() {
  return get_int_pref("enabled");
}

void set_device_enabled(bool flag) {
  set_int_pref("enabled", flag);
}

const char* location_name() {
  return get_string_pref("location");
}

void set_location_name(const char* name) {
  set_string_pref("location", name);
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

#ifdef TIMESERVER
const char* time_server_host() {
  return get_string_pref("time-server-host");
}

int time_server_port() {
  return get_int_pref("time-server-port");
}
#endif

#ifdef MQTT_UPLOAD
unsigned long mqtt_capture_interval_s() {
  if (slideshow_mode) {
    return MQTT_SLIDESHOW_CAPTURE_INTERVAL_S;
  } else {
    return mqtt_monitoring_capture_interval_s;
  }
}

void set_mqtt_capture_interval_s(unsigned long interval) {
  mqtt_monitoring_capture_interval_s = interval;
}

const char* mqtt_endpoint_host() {
  return get_string_pref("aws-iot-endpoint-host");
}

int mqtt_endpoint_port() {
  return get_int_pref("aws-iot-endpoint-port");
}

const char* mqtt_device_id() {
  return get_string_pref("aws-iot-id");
}

const char* mqtt_device_class() {
  return get_string_pref("aws-iot-class");
}

const char* mqtt_root_ca_cert() {
  return get_string_pref("aws-iot-root-ca");
}

const char* mqtt_device_cert() {
  return get_string_pref("aws-iot-device-cert");
}

const char* mqtt_device_private_key() {
  return get_string_pref("aws-iot-private-key");
}
#endif

#ifdef WEB_CONFIGURATION
const char* web_config_access_point() {
  return get_string_pref("web-config-access-point");
}
#endif
