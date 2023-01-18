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

// Configuration file format version, see the 'config' statement help text further down.
// This is intended to be used in a proper "semantic versioning" way.
//
// Version 1.1:
//   Added web-config-access-point

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUGFIX_VERSION 0

// All the str_values in a Configuration point to individual malloc'd NUL-terminated strings.
// By and large none will have leading or trailing whitespace unless they were defined by
// quoted strings that include such whitespace.

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

#ifdef DEVELOPMENT
# define IF_DEVEL(x, y) x
#else
# define IF_DEVEL(x, y) y
#endif
#if defined(DEVELOPMENT) && defined(TIMESTAMP)
# define IF_TIMESTAMP(x, y) x
#else
# define IF_TIMESTAMP(x, y) y
#endif
#if defined(DEVELOPMENT) && defined(WEB_UPLOAD)
# define IF_HTTP_UP(x, y) x
#else
# define IF_HTTP_UP(x, y) y
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
  {"time-server-host",        "tsh",   Pref::Str,              0, IF_TIMESTAMP(TIME_SERVER_HOST, ""),    "Host name of ad-hoc time server"},
  {"time-server-port",        "tsp",   Pref::Int,              IF_TIMESTAMP(TIME_SERVER_PORT, 8086), "", "Port name on the ad-hoc time server"},
  {"http-upload-host",        "huh",   Pref::Str,              0, IF_HTTP_UP(WEB_UPLOAD_HOST, ""),       "Host name of ad-hoc http sensor-reading upload server"},
  {"http-upload-port",        "hup",   Pref::Int,              IF_HTTP_UP(WEB_UPLOAD_PORT, 8086), "",    "Port number on the ad-hoc http sensor-reading upload server"},
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

static void reset_configuration() {
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

static Pref* get_pref(const char* name) {
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

// Non-configurable preferences

#define MINUTE(s) ((s)*60)
#define HOUR(s) ((s)*60*60)

#if defined(SLIDESHOW_MODE) || defined(DEVELOPMENT)
static const unsigned long SENSOR_POLL_INTERVAL_S = 15;
#else
# ifdef TEST_POWER_MANAGEMENT
static const unsigned long SENSOR_POLL_INTERVAL_S = MINUTE(5);
# else
static const unsigned long SENSOR_POLL_INTERVAL_S = HOUR(1);
# endif
#endif

#ifdef MQTT_UPLOAD
# if defined(SLIDESHOW_MODE) || defined(DEVELOPMENT)
static const unsigned long MQTT_CAPTURE_INTERVAL_S = MINUTE(1);
static const unsigned long MQTT_UPLOAD_INTERVAL_S = MINUTE(2);
# else
#  ifdef TEST_POWER_MANAGEMENT
static const unsigned long MQTT_CAPTURE_INTERVAL_S = MINUTE(5);
#  else
static const unsigned long MQTT_CAPTURE_INTERVAL_S = HOUR(1);
#  endif
static const unsigned long MQTT_UPLOAD_INTERVAL_S = MQTT_CAPTURE_INTERVAL_S;
# endif
static const unsigned long MQTT_MAX_IDLE_TIME_S = 30;
#endif

#ifdef SLIDESHOW_MODE
static const unsigned long SLIDESHOW_UPDATE_INTERVAL_S = 4;
#endif

#ifdef WEB_SERVER
static const int WEB_SERVER_LISTEN_PORT = 8088;
static const unsigned long WEB_SERVER_POLL_INTERVAL_S = 1;
#endif

#ifdef WEB_UPLOAD
# ifdef DEVELOPMENT
static const unsigned long WEB_UPLOAD_INTERVAL_S = MINUTE(1);
# else
static const unsigned long WEB_UPLOAD_INTERVAL_S = HOUR(1);
# endif
#endif

#ifdef SNAPPY_SERIAL_LINE
static const unsigned long SERIAL_LINE_POLL_INTERVAL_S = 1;
#endif

static struct {
  unsigned long mqtt_capture_interval_s;
} builtin_cfg = {
  .mqtt_capture_interval_s = MQTT_CAPTURE_INTERVAL_S
};

// Preference accessors

unsigned long sensor_poll_interval_s() {
  return SENSOR_POLL_INTERVAL_S;
}

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

#ifdef TIMESTAMP
const char* time_server_host() {
  return get_string_pref("time-server-host");
}

int time_server_port() {
  return get_int_pref("time-server-port");
}
#endif

#ifdef WEB_UPLOAD
const char* web_upload_host() {
  return get_string_pref("http-upload-host");
}

int web_upload_port() {
  return get_int_pref("http-upload-port");
}

unsigned long web_upload_interval_s() {
  return WEB_UPLOAD_INTERVAL_S;
}
#endif

#ifdef MQTT_UPLOAD
unsigned long mqtt_capture_interval_s() {
  return builtin_cfg.mqtt_capture_interval_s;
}

void set_mqtt_capture_interval_s(unsigned long interval) {
  builtin_cfg.mqtt_capture_interval_s = interval;
}

unsigned long mqtt_max_idle_time_s() {
  return MQTT_MAX_IDLE_TIME_S;
}

unsigned long mqtt_upload_interval_s() {
  return MQTT_UPLOAD_INTERVAL_S;
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

#ifdef SLIDESHOW_MODE
unsigned long slideshow_update_interval_s() {
  return SLIDESHOW_UPDATE_INTERVAL_S;
}
#endif

#ifdef WEB_SERVER
int web_server_listen_port() {
  return WEB_SERVER_LISTEN_PORT;
}

unsigned long web_command_poll_interval_s() {
  return WEB_SERVER_POLL_INTERVAL_S;
}
#endif

#ifdef SNAPPY_SERIAL_LINE
unsigned long serial_line_poll_interval_s() {
  return SERIAL_LINE_POLL_INTERVAL_S;
}
#endif

#ifdef WEB_CONFIGURATION
const char* web_config_access_point() {
  return get_string_pref("web-config-access-point");
}
#endif

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

// evaluate_config() evaluates a configuration program, using the `read_line` parameter
// to read lines of input from some source of text.  It returns a String, which is empty
// if everything was fine and is otherwise an error message.
//
// The configuration language is described by the manual below.

static String evaluate_config(List<String>& input) {
  for (;;) {
    if (input.is_empty()) {
      return fmt("Configuration program did not end with `end`");
    }
    String line = input.pop_front();
    String kwd = get_word(line, 0);
    if (kwd == "end") {
      return String();
    } else if (kwd == "clear") {
      reset_configuration();
    } else if (kwd == "save") {
      save_configuration();
    } else if (kwd == "version") {
      int major, minor, bugfix;
      if (sscanf(get_word(line, 1).c_str(), "%d.%d.%d", &major, &minor, &bugfix) != 3) {
        return fmt("Bad statement [%s]\n", line.c_str());
      }
      if (major != MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION)) {
        // Ignore the bugfix version for now, but require it in the input
        return fmt("Bad version %d.%d.%d, I'm %d.%d.%d\n",
                   major, minor, bugfix,
                   MAJOR_VERSION, MINOR_VERSION, BUGFIX_VERSION);
      }
      // version is OK
    } else if (kwd == "set") {
      String varname = get_word(line, 1);
      if (varname == "") {
        return fmt("Missing variable name for 'set'\n");
      }
      // It's legal to use "" as a value, but illegal not to have a value.
      bool flag = false;
      String value = get_word(line, 2, &flag);
      if (!flag) {
        return fmt("Missing value for variable [%s]\n", varname.c_str());
      }
      Pref* p = get_pref(varname.c_str());
      if (p == nullptr || p->is_cert()) {
        return fmt("Unknown or inappropriate variable name for 'set': [%s]\n", varname.c_str());
      }
      if (p->is_string()) {
        p->str_value = value;
      } else {
        p->int_value = int(value.toInt());
      }
    } else if (kwd == "cert") {
      String varname = get_word(line, 1);
      if (varname == "") {
        return fmt("Missing variable name for 'cert'\n");
      }
      String value;
      if (input.is_empty()) {
        return fmt("Unexpected end of input in config (certificate)");
      }
      String line = input.pop_front();
      if (!line.startsWith("-----BEGIN ")) {
        return fmt("Expected -----BEGIN at the beginning of cert");
      }
      value = line;
      value += "\n";
      for (;;) {
        if (input.is_empty()) {
          return fmt("Unexpected end of input in config (certificate)");
        }
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
        return fmt("Unknown or inappropriate variable name for 'cert': [%s]\n", varname.c_str());
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
        return fmt("Bad configuration statement [%s]\n", line.c_str());
      }
    }
  }
  /*NOTREACHED*/
  panic("Unreachable state in evaluate_config");
}

#ifdef INTERACTIVE_CONFIGURATION

// Manual for the configuration language.
//
// The configuration language is used only during provisioning at this point.

static const char CONFIG_MANUAL_PART1[] = R"EOF(
config
  This command will wait for you to enter a program in a simple configuration language,
  as follows.  Generally whitespace is insignificant except within quoted values
  and in the payloads for `cert`.  Comment lines start with # and go until EOL.
  Statements are executed in the order they appear.

    Program ::= Statements End
    End ::= "end" EOL
    Statement ::= Clear | Version | Save | Set | Cert
    Clear ::= "clear" EOL
      -- This resets all variables
    Version ::= "version" Major-dot-Minor-dot-Bugfix EOL
      -- This optional statement states the version of the firmware that the
      -- configuration was written for.
    Save ::= "save" EOL
      -- This saves all variables to nonvolatile storage.
    Set ::= "set" Variable Value EOL
      -- This assigns Value to Variable
      -- Value is a string of non-whitespace characters, or a quoted string
    Cert ::= "cert" Cert-name EOL Text-lines
      -- This defines a multi-line text variable
      -- The first payload line must start with the usual "-----BEGIN ...", and the last
      -- payload line must end with with the usual "-----END ...".  No blank lines or
      -- comments may appear after the "cert" line until after the last payload line.)EOF";

// "Part 2" of the manual is the list of variable names for `set` and `cert`; see later.

static void print_help(Stream* io) {
  io->println(R"EOF(Interactive configuration commands:

show
  Show current settings.  Certificates are shown with first line only,
  passwords with first letter only.

config
  This will wait for you to enter (or more usually, paste in) a number of lines
  followed by a line that reads only "end".  See `help config`.

clear
  Remove all settings (or restore them to compiled-in values, in development mode).

save
  Save the configuration to nonvolatile storage.

quit
  Leave configuration mode and start the device in a normal manner.

Note it is possible to compile default settings into the source for easier development,
see src/config.cpp.)EOF");
}

static void print_help_config(Stream* io) {
  io->println(CONFIG_MANUAL_PART1);
  io->println();
  io->println("  Variables for 'set' are:");
  for ( Pref* p = prefs; p->long_key != nullptr; p++ ) {
    if (!p->is_cert()) {
      io->printf("    %-22s - %s\n", p->long_key, p->help);
    }
  }
  io->println();
  io->println("  Cert-names for 'cert' are:");
  for ( Pref* p = prefs; p->long_key != nullptr; p++ ) {
    if (p->is_cert()) {
      io->printf("    %-20s - %s\n", p->long_key, p->help);
    }
  }
}

static String cert_first_line(const char* cert) {
  const char* p = strstr(cert, "BEGIN");
  const char* q = strchr(p, '\n');
  const char* r = strchr(q+1, '\n');
  return String((const uint8_t*)(q+1), (r-q-1));
}

static void show_cmd(Stream* io) {
  for (Pref* p = prefs; p->long_key != nullptr; p++ ) {
    if (p->is_string()) {
      if (p->str_value.isEmpty()) {
        continue;
      }
      if (p->is_cert()) {
        io->printf("%-22s - %s...\n", p->long_key, cert_first_line(p->str_value.c_str()).c_str());
      } else if (p->is_passwd() && !p->str_value.isEmpty()) {
        io->printf("%-22s - %c.....\n", p->long_key, p->str_value[0]);
      } else {
        io->printf("%-22s - %s\n", p->long_key, p->str_value.c_str());
      }
    } else {
      io->printf("%-22s - %d\n", p->long_key, p->int_value);
    }
  }
}
#endif  // INTERACTIVE_CONFIGURATION

#ifdef INTERACTIVE_CONFIGURATION
void ReadSerialConfigInputTask::perform() {
  auto *io = &Serial;
  if (state == COLLECTING) {
    // collecting input for the "config" command in `config_lines`, ending when we've seen
    // the "end" line.
    String cmd = get_word(line, 0);
    config_lines.add_back(std::move(line));
    if (cmd == "end") {
      String result = evaluate_config(config_lines);
      if (!result.isEmpty()) {
        io->println(result);
      }
      config_lines.clear();
      state = RUNNING;
    }
  } else {
    String cmd = get_word(line, 0);
    if (cmd == "help") {
      if (get_word(line, 1).equals("config")) {
        print_help_config(io);
      } else {
        print_help(io);
      }
    } else if (cmd == "show") {
      show_cmd(io);
    } else if (cmd == "config") {
      state = COLLECTING;
    } else if (cmd == "clear") {
      reset_configuration();
    } else if (cmd == "save") {
      save_configuration();
    } else if (cmd == "quit") {
      io->println("PRESS RESET BUTTON");
      enter_end_state("PRESS RESET BUTTON");
    } else {
      io->printf("Unknown command [%s], try `help`.\n", line.c_str());
    }
  }
}
#endif
