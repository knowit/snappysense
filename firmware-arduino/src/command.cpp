// Interactive commands

#include "command.h"

#ifdef SNAPPY_COMMAND_PROCESSOR

#include "config.h"
#include "util.h"

static void cmd_hello(const String& cmd, const SnappySenseData&, Stream& out);
static void cmd_help(const String& cmd, const SnappySenseData&, Stream& out);
static void cmd_get(const String& cmd, const SnappySenseData&, Stream& out);
static void cmd_view(const String& cmd, const SnappySenseData&, Stream& out);
static void cmd_inet(const String& cmd, const SnappySenseData&, Stream& out);
static void cmd_config(const String& cmd, const SnappySenseData&, Stream& out);

struct Command {
  const char* command;
  const char* help;
  void (*handler)(const String& cmd, const SnappySenseData& data, Stream&);
};

// The last row of this table has a null `command` field
static Command commands[] = {
  {"hello",    "Echo the argument",                                  cmd_hello},
  {"help",     "Print help text",                                    cmd_help},
  {"get",      "Get the sensor reading for a specific sensor name",  cmd_get},
  {"view",     "View all the current sensor readings",               cmd_view},
  {"inet",     "Internet connectivity details",                      cmd_inet},
  {"config",   "Show device configuration",                          cmd_config},
  {nullptr,    nullptr,                                              nullptr}
};

void command_evaluate(const String& cmd, const SnappySenseData& data, Stream& out) {
  String w = get_word(cmd, 0);
  if (!w.isEmpty()) {
    for (Command* c = commands; c->command != nullptr; c++ ) {
      if (strcmp(c->command, w.c_str()) == 0) {
        c->handler(cmd, data, out);
        return;
      }
    }
  }
  out.printf("Unrecognized command [%s]\n", cmd.c_str());
}

static void cmd_hello(const String& cmd, const SnappySenseData&, Stream& out) {
  String arg = get_word(cmd, 1);
  if (!arg.isEmpty()) {
    out.printf("Hello %s\n", arg.c_str());
  } else {
    out.println("Hello, whoever you are\n");
  }
}

static void cmd_help(const String& cmd, const SnappySenseData&, Stream& out) {
  out.println("Commands:");
  for (Command* c = commands; c->command != nullptr; c++ ) {
    out.printf(" %s - %s\n", c->command, c->help);
  }
  out.println("\nSensor names for `get` are:");
  for (SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++) {
    out.printf(" %s - %s (%s)\n", m->json_key, m->explanatory_text, m->unit_text);
  }
}

static void cmd_view(const String& cmd, const SnappySenseData& data, Stream& out) {
  out.println("Measurement Data");
  out.println("----------------");
  for ( SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++ ) {
    // Skip data that are not valid
    if (m->flag_offset > 0 &&
        !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&data) + m->flag_offset)) {
      continue;
    }
    // This ignores m->display on purpose
    out.print(m->explanatory_text);
    out.print(": ");
    char buf[32];
    m->format(data, buf, buf+sizeof(buf));
    out.print(buf);
    out.print(" ");
    out.println(m->unit_text);
  }
}

static void cmd_get(const String& cmd, const SnappySenseData& data, Stream& out) {
  String arg = get_word(cmd, 1);
  if (arg.isEmpty()) {
    out.println("Sensor name needed, try `help`");
    return;
  }
  for ( SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++ ) {
    if (strcmp(m->json_key, arg.c_str()) == 0) {
      // Skip data that are not valid
      if (m->flag_offset > 0 &&
          !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&data) + m->flag_offset)) {
        out.printf("%s: data not valid\n", m->json_key);
      } else {
        char buf[32];
        m->format(data, buf, buf+sizeof(buf));
        out.printf("%s: %s\n", m->json_key, buf);
      }
      return;
    }
  }
  out.println("Invalid sensor name, try `help`");
}

static void cmd_inet(const String& cmd, const SnappySenseData&, Stream& out) {
#ifdef SNAPPY_MQTT
  out.println("MQTT upload is enabled");
#endif
#ifdef SNAPPY_WEBCONFIG
  out.printf("Web configuration is enabled, ap %s\n", web_config_access_point());
#endif
#ifdef SNAPPY_I2CCONFIG
  out.printf("I2C configuration is enabled, address TBD\n");
#endif
}

static void cmd_config(const String& cmd, const SnappySenseData&, Stream& out) {
  show_configuration(&out);
}

#endif // SNAPPY_COMMAND_PROCESSOR
