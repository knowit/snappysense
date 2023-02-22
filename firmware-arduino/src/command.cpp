// Interactive commands for serial port and web server

#include "command.h"

#ifdef SNAPPY_COMMAND_PROCESSOR

#include "config.h"
#include "device.h"
#include "network.h"
#include "util.h"
#include "web_server.h"

struct Command {
  const char* command;
  const char* help;
  void (*handler)(const String& cmd, const SnappySenseData& data, Stream*);
};

// The last row of this table has a null `command` field
extern Command commands[];

void execute_command(Stream* output, String cmd, SnappySenseData* data) {
  String w = get_word(cmd, 0);
  if (!w.isEmpty()) {
    for (Command* c = commands; c->command != nullptr; c++ ) {
      if (strcmp(c->command, w.c_str()) == 0) {
        c->handler(cmd, *data, output);
        return;
      }
    }
  }
  output->printf("Unrecognized command [%s]\n", cmd.c_str());
}

static void cmd_hello(const String& cmd, const SnappySenseData&, Stream* out) {
  String arg = get_word(cmd, 1);
  if (!arg.isEmpty()) {
    out->printf("Hello %s\n", arg.c_str());
  } else {
    out->println("Hello, whoever you are\n");
  }
}

static void cmd_help(const String& cmd, const SnappySenseData&, Stream* out) {
  out->println("Commands:");
  for (Command* c = commands; c->command != nullptr; c++ ) {
    out->printf(" %s - %s\n", c->command, c->help);
  }
  out->println("\nSensor names for `get` are:");
  for (SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++) {
    out->printf(" %s - %s (%s)\n", m->json_key, m->explanatory_text, m->unit_text);
  }
}

static void cmd_view(const String& cmd, const SnappySenseData& data, Stream* out) {
  out->println("Measurement Data");
  out->println("----------------");
  for ( SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++ ) {
    // Skip data that are not valid
    if (m->flag_offset > 0 &&
        !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&data) + m->flag_offset)) {
      continue;
    }
    // This ignores m->display on purpose
    out->print(m->explanatory_text);
    out->print(": ");
    char buf[32];
    m->format(data, buf, buf+sizeof(buf));
    out->print(buf);
    out->print(" ");
    out->println(m->unit_text);
  }
}

static void cmd_get(const String& cmd, const SnappySenseData& data, Stream* out) {
  String arg = get_word(cmd, 1);
  if (arg.isEmpty()) {
    out->println("Sensor name needed, try `help`");
    return;
  }
  for ( SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++ ) {
    if (strcmp(m->json_key, arg.c_str()) == 0) {
      // Skip data that are not valid
      if (m->flag_offset > 0 &&
          !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&data) + m->flag_offset)) {
        out->printf("%s: data not valid\n", m->json_key);
      } else {
        char buf[32];
        m->format(data, buf, buf+sizeof(buf));
        out->printf("%s: %s\n", m->json_key, buf);
      }
      return;
    }
  }
  out->println("Invalid sensor name, try `help`");
}

static void cmd_inet(const String& cmd, const SnappySenseData&, Stream* out) {
#ifdef WEB_UPLOAD
  out->println("Web upload is enabled");
#endif
#ifdef MQTT_UPLOAD
  out->println("MQTT upload is enabled");
#endif
#ifdef WEB_SERVER
  out->printf("Web server is enabled, inet address %s\n", local_ip_address().c_str());
#endif
#ifdef WEB_COMMAND_SERVER
  out->println("Web commands are accepted\n");
#endif
#ifdef WEB_CONFIGURATION
  out->println("Web configuration is enabled, ap %s\n", web_config_access_point());
#endif
}

static void cmd_config(const String& cmd, const SnappySenseData&, Stream* out) {
  show_configuration(out);
}

Command commands[] = {
  {"hello",    "Echo the argument",                                  cmd_hello},
  {"help",     "Print help text",                                    cmd_help},
  {"get",      "Get the sensor reading for a specific sensor name",  cmd_get},
  {"view",     "View all the current sensor readings",               cmd_view},
  {"inet",     "Internet connectivity details",                      cmd_inet},
  {"config",   "Show device configuration",                          cmd_config},
  {nullptr,    nullptr,                                              nullptr}
};

#endif // SNAPPY_COMMAND_PROCESSOR

#ifdef WEB_COMMAND_SERVER

// TODO: Clean up doc

// Command processing:
//
// The device is connected to an external access point, and the device has an IP
// address provided by that AP.  It listens on a port, default 8086, for GET
// requests that ...
//
// TODO: The IP must be shown on the device screen, not just on the serial port.
// It can be shown by the slideshow if the slideshow is running, otherwise whenever
// the device wakes up to read the sensors.
//
// Parameters are passed using the usual syntax, so <http://.../get?temperature> to
// return the temperature reading.  The parameter handling is ad-hoc and works only
// for these simple cases.

// This keeps the client alive until the command task has provided
// output.
class ProcessCommandTaskFromWeb : public ProcessCommandTask {
  WebRequestHandler* cl;
public:
  ProcessCommandTaskFromWeb(WebRequestHandler* cl) : ProcessCommandTask(cl->request, &cl->client), cl(cl) {}
  ~ProcessCommandTaskFromWeb() {
    cl->dead = true;
  }
};

void WebCommandClient::process_request() {
  log("Processing %s\n", request.c_str());
  if (request.startsWith("GET /")) {
    log("Web server: handling GET\n");
    String r = request.substring(5);
    int idx = r.indexOf(' ');
    if (idx != -1) {
      r = r.substring(0, idx);
      // TODO: Issue 24: technically this is url-encoded and needs to be decoded
    }
    if (r.isEmpty()) {
      r = String("help");
    }
    // Hack
    idx = r.indexOf('?');
    if (idx != -1) {
      r[idx] = ' ';
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<pre>");
    sched_microtask_after(new ProcessCommandTaskFromWeb(this), 0);
    client.println("</pre>");
    return;
  }
  log("Web server: invalid method or URL\n");
  client.println("HTTP/1.1 403 Forbidden");
}

void WebCommandClient::failed_request() {
  log("Web server: Incomplete request [%s]\n", request.c_str());
  dead = true;
}

struct WebCommandServer : public WebServer {
  WiFiHolder server_holder;
  WebCommandServer(int port) : WebServer(port) {}
};

bool WebCommandTask::start() {
  int port = web_server_listen_port();
  auto* state = new WebCommandServer(port);
  state->server_holder = connect_to_wifi();
  if (!state->server_holder.is_valid()) {
    // TODO: Does somebody need to know?
    log("Failed to bring up web server\n");
    delete state;
    return false;
  }
  state->server.begin();
  log("Web server: listening on port %d\n", port);
  this->web_server = state;
  return true;
}

#endif
