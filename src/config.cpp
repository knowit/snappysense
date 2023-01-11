// SnappySense configuration manager

// TODO: It would be nice to make this a little more table-driven, as it is, many places
// must be updated when a config variable is added.

#include "config.h"
#include "log.h"
#include "util.h"

#include <functional>
#include <Preferences.h>

// Configuration parameters.

#ifdef DEVELOPMENT
// The file client_config.h provides compiled-in values so as to simplify development.
//
// Note, client_config.h is not in the repo, as it contains private values.  DO NOT ADD IT.
// A template for the file is in client_config_template.txt.  In that file, search for FIXME.
# include "client_config.h"
#endif

// Configuration file format version, see the 'conf' statement help text further down.
// This is intended to be used in a proper "semantic versioning" way.

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUGFIX_VERSION 0

// All the char* values in a Configuration point to individual malloc'd NUL-terminated strings.
// By and large none will have leading or trailing whitespace unless they were defined by
// quoted strings that include such whitespace.

struct Configuration {
  bool  device_enabled;
  char* location_name;
  char* ssid1;
  char* ssid2;
  char* ssid3;
  char* password1;
  char* password2;
  char* password3;
  char* time_server_host;
  int   time_server_port;
  char* http_upload_host;
  int   http_upload_port;
  char* aws_iot_id;
  char* aws_iot_class;
  char* aws_iot_endpoint_host;
  int   aws_iot_endpoint_port;
  char* aws_iot_root_ca;
  char* aws_iot_device_cert;
  char* aws_iot_private_key;
};

// Free anything pointed to by *var, then allocate memory for newstr and
// copy newstr into it, placing it in *var.

static void replace(char** var, const char* newstr) {
  free(*var);
  *var = strdup(newstr);
}

// The default ("factory") configuration takes its values from the compiled-in
// configuration if there is one, otherwise it's mostly empty strings.

static Configuration default_configuration = {
  .device_enabled = true,
#ifdef DEVELOPMENT
  .location_name = strdup(LOCATION_NAME),
#else
  .location_name = strdup(""),
#endif
#ifdef DEVELOPMENT
  .ssid1 = strdup(WIFI_SSID),
#else
  .ssid1 = strdup(""),
#endif
  .ssid2 = strdup(""),
  .ssid3 = strdup(""),
#ifdef DEVELOPMENT
  .password1 = strdup(WIFI_PASSWORD),
#else
  .password1 = strdup(""),
#endif
  .password2 = strdup(""),
  .password3 = strdup(""),
#if defined(DEVELOPMENT) && defined(TIMESTAMP)
  .time_server_host = strdup(TIME_SERVER_HOST),
  .time_server_port = TIME_SERVER_PORT,
#else
  .time_server_host = strdup(""),
  .time_server_port = 8086,
#endif
#if defined(DEVELOPMENT) && defined(WEB_UPLOAD)
  .http_upload_host = strdup(WEB_UPLOAD_HOST),
  .http_upload_port = WEB_UPLOAD_PORT,
#else
  .http_upload_host = strdup(""),
  .http_upload_port = 8086,
#endif
#if defined(DEVELOPMENT) && defined(MQTT_UPLOAD)
  .aws_iot_id = strdup(AWS_CLIENT_IDENTIFIER),
  .aws_iot_class = strdup("snappysense"),
  .aws_iot_endpoint_host = strdup(AWS_IOT_ENDPOINT),
  .aws_iot_endpoint_port = AWS_MQTT_PORT,
  .aws_iot_root_ca = strdup(AWS_CERT_CA),
  .aws_iot_device_cert = strdup(AWS_CERT_CRT),
  .aws_iot_private_key = strdup(AWS_CERT_PRIVATE),
#else
  .aws_iot_id = strdup(""),
  .aws_iot_class = strdup("snappysense"),
  .aws_iot_endpoint = strdup(""),
  .aws_iot_endpoint_port = 8883,
  .aws_iot_root_ca = strdup(""),
  .aws_iot_device_cert = strdup(""),
  .aws_iot_private_key = strdup(""),
#endif
};

// The current configuration object.  There is only one.

static Configuration cfg;

// Copy the default configuration into the current configuration.

static void reset_configuration() {
  cfg.device_enabled = default_configuration.device_enabled,
  replace(&cfg.location_name, default_configuration.location_name);
  replace(&cfg.ssid1, default_configuration.ssid1);
  replace(&cfg.ssid2, default_configuration.ssid2);
  replace(&cfg.ssid3, default_configuration.ssid3);
  replace(&cfg.password1, default_configuration.password1);
  replace(&cfg.password2, default_configuration.password2);
  replace(&cfg.password3, default_configuration.password3);
  replace(&cfg.time_server_host, default_configuration.time_server_host);
  cfg.time_server_port = default_configuration.time_server_port;
  replace(&cfg.http_upload_host, default_configuration.http_upload_host);
  cfg.http_upload_port = default_configuration.http_upload_port;
  replace(&cfg.aws_iot_id, default_configuration.aws_iot_id);
  replace(&cfg.aws_iot_class, default_configuration.aws_iot_class);
  replace(&cfg.aws_iot_endpoint_host, default_configuration.aws_iot_endpoint_host);
  cfg.aws_iot_endpoint_port = default_configuration.aws_iot_endpoint_port;
  replace(&cfg.aws_iot_root_ca, default_configuration.aws_iot_root_ca);
  replace(&cfg.aws_iot_device_cert, default_configuration.aws_iot_device_cert);
  replace(&cfg.aws_iot_private_key, default_configuration.aws_iot_private_key);
};

// Non-configurable preferences

#ifdef DEVELOPMENT
static const unsigned long SENSOR_POLL_FREQUENCY_S = 15;
#else
static const unsigned long SENSOR_POLL_FREQUENCY_S = 60;
#endif

#ifdef WEB_SERVER
static const int WEB_SERVER_LISTEN_PORT = 8088;
static const unsigned long WEB_SERVER_WAIT_TIME_S = 1;
#endif

#ifdef WEB_UPLOAD
// TODO: The upload frequency should ideally be a multiple of the sensor
// poll frequency; and/or there should be no upload if the sensor
// has not been read since the last time. 
#ifdef DEVELOPMENT
static const unsigned long WEB_UPLOAD_WAIT_TIME_S = 60*1;
#else
static const unsigned long WEB_UPLOAD_WAIT_TIME_S = 60*60; // 1 hour
#endif
#endif

#ifdef MQTT_UPLOAD
#ifdef DEVELOPMENT
static const unsigned long MQTT_UPLOAD_WAIT_TIME_S = 60*1;
static const unsigned long MQTT_SLEEP_INTERVAL_S = 60*2;
#else
static const unsigned long MQTT_UPLOAD_WAIT_TIME_S = 60*60; // 1 hour
static const unsigned long MQTT_SLEEP_INTERVAL_S = MQTT_UPLOAD_WAIT_TIME_S 
#endif
static const unsigned long MQTT_MAX_IDLE_TIME_S = 30;
static unsigned long mqtt_upload_wait_time_s = MQTT_UPLOAD_WAIT_TIME_S;
#endif

#ifdef STANDALONE
static const unsigned long SCREEN_WAIT_TIME_S = 4;
#endif

#ifdef SERIAL_SERVER
static const unsigned long SERIAL_SERVER_WAIT_TIME_S = 1;
#endif

// Preference accessors

unsigned long sensor_poll_frequency_seconds() {
  return SENSOR_POLL_FREQUENCY_S;
}

bool device_enabled() {
  return cfg.device_enabled;
}

void set_device_enabled(bool flag) {
  cfg.device_enabled = flag;
}

const char* location_name() {
  return cfg.location_name;
}

const char* access_point_ssid(int n) {
  switch (n) {
    case 1: return cfg.ssid1;
    case 2: return cfg.ssid2;
    case 3: return cfg.ssid3;
    default: return "";
  }
}

const char* access_point_password(int n) {
  switch (n) {
    case 1: return cfg.password1;
    case 2: return cfg.password2;
    case 3: return cfg.password3;
    default: return "";
  }
}

#ifdef TIMESTAMP
const char* time_server_host() {
  return cfg.time_server_host;
}

int time_server_port() {
  return cfg.time_server_port;
}
#endif

#ifdef WEB_UPLOAD
const char* web_upload_host() {
  return cfg.http_upload_host;
}

int web_upload_port() {
  return cfg.http_upload_port;
}

unsigned long web_upload_frequency_seconds() {
  return WEB_UPLOAD_WAIT_TIME_S;
}
#endif

#ifdef MQTT_UPLOAD
unsigned long mqtt_capture_frequency_seconds() {
  return mqtt_upload_wait_time_s;
}

void set_mqtt_capture_frequency_seconds(unsigned long interval) {
  mqtt_upload_wait_time_s = interval;
}

unsigned long mqtt_max_idle_time_seconds() {
  return MQTT_MAX_IDLE_TIME_S;
}

unsigned long mqtt_sleep_interval_seconds() {
  return MQTT_SLEEP_INTERVAL_S;
}

const char* mqtt_endpoint_host() {
  return cfg.aws_iot_endpoint_host;
}

int mqtt_endpoint_port() {
  return cfg.aws_iot_endpoint_port;
}

const char* mqtt_device_id() {
  return cfg.aws_iot_id;
}

const char* mqtt_device_class() {
  return cfg.aws_iot_class;
}

const char* mqtt_root_ca_cert() {
  return cfg.aws_iot_root_ca;
}

const char* mqtt_device_cert() {
  return cfg.aws_iot_device_cert;
}

const char* mqtt_device_private_key() {
  return cfg.aws_iot_private_key;
}
#endif

#ifdef STANDALONE
unsigned long display_update_frequency_seconds() {
  return SCREEN_WAIT_TIME_S;
}
#endif

#ifdef WEB_SERVER
int web_server_listen_port() {
  return WEB_SERVER_LISTEN_PORT;
}

unsigned long web_command_poll_seconds() {
  return WEB_SERVER_WAIT_TIME_S;
}
#endif

#ifdef SERIAL_SERVER
unsigned long serial_command_poll_seconds() {
  return SERIAL_SERVER_WAIT_TIME_S;
}
#endif

// Provisioning and run-time configuration.

// Format stuff into a String.
static String fmt(const char* format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  return String(buf);
}

// Manual for the configuration language.
//
// The configuration language is used both for user-entered options during provisioning
// and to persist the configuration in NVRAM.  The function generate_config() generates
// a program from a set of preferences.

static const char CONFIG_MANUAL[] = R"EOF(
config
  This command will wait for you to enter a program in a simple configuration language,
  as follows.  Generally whitespace is insignificant except within quoted values
  and in the payloads for `cert`.  Comment lines start with # and go until EOL.
  Statements are executed in the order they appear.

    Program ::= Statement End
    End ::= "end" EOL
    Statement ::= Clear | Version | Set | Cert
    Clear ::= "clear" EOL
      -- This resets all variables
    Version ::= "version" Major-dot-Minor EOL
      -- This optional statement states the version of the firmware that the
      -- configuration was written for.
    Set ::= "set" Variable Value EOL
      -- This assigns Value to Variable
      -- Value is a string of non-whitespace characters, or a quoted string
    Cert ::= "cert" Cert-name EOL Text-lines
      -- This defines a multi-line text variable
      -- The first payload line must start with the usual "-----BEGIN ...", and the last
      -- payload line must end with with the usual "-----END ...".  No blank lines or
      -- comments may appear after the "cert" line until after the last payload line.

  Variables for 'set' are:
    ssid1, ssid2, ssid3             - WiFi ssid names for up to 3 networks
    password1, password2, password3 - WiFi passwords, ditto
    location                        - name of device location
    time-server-host                - host name of ad-hoc time server
    aws-iot-id                      - IoT device ID
    aws-iot-class                   - IoT device class (default "snappysense")
    aws-iot-endpoint-host           - IoT endpoint host name
    aws-iot-endpoint-port           - IoT port number (default 8883)
    http-upload-host                - host name for http sensor uploads
    http-upload-port                - port name for http sensor uploads
  
  Cert-names for 'cert' are:
    aws-iot-root-ca                 - Root CA certificate (AmazonRootCA1.pem)
    aws-iot-device-cert             - Device certificate (XXXXXXXXXX-certificate.pem.crt)
    aws-iot-private-key             - Private key (XXXXXXXXXX-private.pem.key))EOF";

// evaluate_config() evaluates a configuration program, using the `read_line` parameter
// to read lines of input from some source of text.  It returns a String, which is empty
// if everything was fine and is otherwise an error message.
//
// The configuration language is described by the manual above.

static String execute_config(std::function<String()> read_line) {
  for (;;) {
    String line = read_line();
    if (line == "") {
      return fmt("Configuration program did not end with `end`");
    }
    String kwd = get_word(line, 0);
    if (kwd == "end") {
      return String();
    } else if (kwd == "clear") {
      reset_configuration();
    } else if (kwd == "version") {
      int major, minor, bugfix;
      if (sscanf(get_word(line, 1).c_str(), "%d.%d,%d", &major, &minor, &bugfix) != 3) {
        return fmt("Bad statement [%s]\n", line.c_str());
      }
      if (major != MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION)) {
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
      String value = get_word(line, 2);
      if (value == "") {
        return fmt("Missing value for variable [%s]\n", varname.c_str());
      }
      if (varname == "ssid1") {
        replace(&cfg.ssid1, value.c_str());
      } else if (varname == "ssid2") {
        replace(&cfg.ssid2, value.c_str());
      } else if (varname == "ssid3") {
        replace(&cfg.ssid3, value.c_str());
      } else if (varname == "password1") {
        replace(&cfg.password1, value.c_str());
      } else if (varname == "password2") {
        replace(&cfg.password2, value.c_str());
      } else if (varname == "password3") {
        replace(&cfg.password3, value.c_str());
      } else if (varname == "location") {
        replace(&cfg.location_name, value.c_str());
      } else if (varname == "time-server-host") {
        replace(&cfg.location_name, value.c_str());
      } else if (varname == "aws-iot-id") {
        replace(&cfg.aws_iot_id, value.c_str());
      } else if (varname == "aws-iot-class") {
        replace(&cfg.aws_iot_class, value.c_str());
      } else if (varname == "aws-iot-endpoint-host") {
        replace(&cfg.aws_iot_endpoint_host, value.c_str());
      } else if (varname == "aws-iot-endpoint-port") {
        // FIXME
      } else if (varname == "http-upload-host") {
        replace(&cfg.http_upload_host, value.c_str());
      } else if (varname == "http-upload-port") {
        // FIXME
      } else {
        return fmt("Unknown variable name for 'set': [%s]\n", varname.c_str());
      }
    } else if (kwd == "cert") {
      String varname = get_word(line, 1);
      if (varname == "") {
        return fmt("Missing variable name for 'cert'\n");
      }
      String value;
      String line = read_line();
      if (!line.startsWith("-----BEGIN ")) {
        return fmt("Expected -----BEGIN at the beginning of cert");
      }
      value = line;
      value += "\n";
      for (;;) {
        String line = read_line();
        value += line;
        value += "\n";
        if (line.startsWith("-----END ")) {
          break;
        }
      }
      value.trim();
      if (varname == "aws-iot-root-ca") {
        replace(&cfg.aws_iot_root_ca, value.c_str());
      } else if (varname == "aws-iot-device-cert") {
        replace(&cfg.aws_iot_device_cert, value.c_str());
      } else if (varname == "aws-iot-private-key") {
        replace(&cfg.aws_iot_private_key, value.c_str());
      }
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
  log("Should not happen");
  abort();
}

// Generate a `set <var> <value>` statement
template<typename T>
static void set(String* s, const char* tag, T value) {
  *s += "set ";
  *s += tag;
  // TODO: Choose quoting based on contents of string - it could
  // require single-quoting.
  *s += " \"";
  *s += value;
  *s += "\"\n";
}

// Generate a `cert <var>` statement with a trailing text block
static void cert(String* s, const char* tag, const char* value) {
  *s += "cert ";
  *s += tag;
  *s += '\n';
  *s += value;
  *s += '\n';
}

// Generate a program that will recreate the current prefs, this is useful as the prefs
// are persisted as this program.
static String generate_prefs() {
  String s;
  s += "clear\n";
  s += "version ";
  s += MAJOR_VERSION;
  s += '.';
  s += MINOR_VERSION;
  s += '.';
  s += BUGFIX_VERSION;
  s += '\n';
  set(&s, "location-name", cfg.location_name);
  set(&s, "ssid1", cfg.ssid1);
  set(&s, "ssid2", cfg.ssid2);
  set(&s, "ssid3", cfg.ssid3);
  set(&s, "password1", cfg.password1);
  set(&s, "password2", cfg.password2);
  set(&s, "password3", cfg.password3);
  set(&s, "time-server-host", cfg.time_server_host);
  // TODO: time-server-port
  set(&s, "http-upload-host", cfg.http_upload_host);
  set(&s, "http-upload-port", cfg.http_upload_port);
  set(&s, "aws-iot-id", cfg.aws_iot_id);
  set(&s, "aws-iot-class", cfg.aws_iot_class);
  set(&s, "aws-iot-endpoint-host", cfg.aws_iot_endpoint_host);
  set(&s, "aws-iot-endpoint-port", cfg.aws_iot_endpoint_port);
  cert(&s, "aws-iot-root-ca", cfg.aws_iot_root_ca);
  cert(&s, "aws-iot-device-cert", cfg.aws_iot_device_cert);
  cert(&s, "aws-iot-private-key", cfg.aws_iot_private_key);
  s += "end\n";
  return s;
}

// This does not work.  The max value size is 4000 bytes, and only if the parameter store is not fragmented.
// For a full config we easily have more than that.

static void save_configuration(Stream* io) {
  String cfg = generate_prefs();
  Preferences prefs;
  if (prefs.begin("snappysense")) {
    log("A");
    prefs.putString("snappyprefs", generate_prefs());
    log("B");
    prefs.end();
    log("C");
  } else {
    io->println("Failed to save configuration\n");
  }
}

void read_configuration(Stream* io) {
  Preferences prefs;
  if (prefs.begin("snappysense", /* readOnly= */ true)) {
    String s = prefs.getString("snappyprefs");
    if (!s.isEmpty()) {
      StringReader rdr(s);
      String result = execute_config([&]{ return blocking_read_nonempty_line(&rdr); });
      if (!result.isEmpty()) {
        io->println(result);
      }
      return;
    }
  }
  log("No configuration in parameter store\n");
  reset_configuration();
}

#ifdef INTERACTIVE_CONFIGURATION

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

gen
  (Development mode only)  Print the config program that would be generated by
  the current configuration.

Note it is possible to compile default settings into the source for easier development,
see src/config.cpp.)EOF");
}

static void print_help_config(Stream* io) {
  io->println(CONFIG_MANUAL);
}

static String cert_first_line(const char* cert) {
  const char* p = strstr(cert, "BEGIN");
  const char* q = strchr(p, '\n');
  const char* r = strchr(q+1, '\n');
  return String((const uint8_t*)(q+1), (r-q-1));
}

static void show_cmd(Stream* io) {
  io->printf("Known SSIDs\n");
  for ( int i=1; i <= 3; i++ ) {
    const char* ssid = access_point_ssid(i);
    const char* pass = access_point_password(i);
    if (*ssid) {
      if (*pass) {
        io->printf(" %s   %c...\n", ssid, *pass);
      } else {
        io->printf(" %s   (no password)\n", ssid);
      }
    }
  }
  io->printf("Location: %s\n", location_name());
#ifdef TIMESTAMP
  io->printf("Time server: %s:%d\n", time_server_host(), time_server_port());
#endif
#ifdef MQTT_UPLOAD
  io->printf("Device class: %s\n", mqtt_device_class());
  io->printf("AWS IoT device ID: %s\n", mqtt_device_id());
  io->printf("AWS IoT endpoint: %s:%d\n", mqtt_endpoint_host(), mqtt_endpoint_port());
  io->printf("AWS Root CA cert: %s\n", cert_first_line(mqtt_root_ca_cert()).c_str());
  io->printf("AWS Device cert: %s\n", cert_first_line(mqtt_device_cert()).c_str());
  io->printf("AWS Private key: %s\n", cert_first_line(mqtt_device_private_key()).c_str());
#endif
#ifdef WEB_UPLOAD
  io->printf("Web upload host: %s:%d\n", web_upload_host(), web_upload_port());
#endif
}

void interactive_configuration(Stream* io) {
  io->print("*** INTERACTIVE CONFIGURATION MODE ***\n\n");
  io->print("Type 'help' for help.\nThere is no line editing - type carefully.\n\n");
  for (;;) {
    String line = blocking_read_nonempty_line(io);
    String cmd = get_word(line, 0);
    if (cmd == "help") {
      if (get_word(line, 1).equals("set") && get_word(line, 2).equals("config")) {
        print_help_config(io);
      } else {
        print_help(io);
      }
    } else if (cmd == "show") {
      show_cmd(io);
    } else if (cmd == "config") {
      String result = execute_config([&]{ return blocking_read_nonempty_line(io); });
      if (!result.isEmpty()) {
        io->println(result);
      }
    } else if (cmd == "clear") {
      reset_configuration();
    } else if (cmd == "save") {
      save_configuration(io);
#ifdef DEVELOPMENT
    } else if (cmd == "gen") {
      io->println(generate_prefs().c_str());
#endif
    } else if (cmd == "quit") {
      break;
    } else {
      io->printf("Unknown command [%s], try `help`.\n", line.c_str());
    }
  }
}
#endif  // INTERACTIVE_CONFIGURATION
