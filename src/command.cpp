// Interactive commands for serial port and web server

#include "command.h"

#if defined(SERIAL_SERVER) || defined(WEB_SERVER)

#include "device.h"
#include "network.h"

struct Command {
  const char* command;
  const char* help;
  void (*handler)(const String& cmd, const SnappySenseData& data, Stream*);
};

// The last row of this table has a null `command` field
extern Command commands[];

// Return nth word, or an empty string if there is no such word
static String get_word(const String& cmd, int n) {
  unsigned lim = cmd.length();
  unsigned i = 0;
  while (i < lim) {
    while (i < lim && isspace(cmd[i])) {
      i++;
    }
    unsigned start = i;
    while (i < lim && !isspace(cmd[i])) {
      i++;
    }
    if (i == start) {
      break;
    }
    if (n == 0) {
      return cmd.substring(start, i);
    }
    n--;
  }
  return String();
}

void ProcessCommandTask::execute(SnappySenseData* data) {
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

static void cmd_scani2c(const String& cmd, const SnappySenseData&, Stream* out) {
  // TODO: turn this into a control task.  Note this may be difficult,
  // as the task cannot hold a reference to the stream - the task may
  // outlive the stream - unless we do something to make sure the stream
  // lives longer, or is safe-for-deletion.
  out->println("Scanning...");
  int num = probe_i2c_devices(out);
  out->print("Number of I2C devices found: ");
  out->println(num, DEC);
  return;  
}

static void cmd_poweron(const String& cmd, const SnappySenseData&, Stream* out) {
  sched_microtask_after(new PowerOnTask, 0);
  out->println("Peripheral power turned on");
}

static void cmd_poweroff(const String& cmd, const SnappySenseData&, Stream* out) {
  sched_microtask_after(new PowerOffTask, 0);
  out->println("Peripheral power turned off");
}

static void cmd_read(const String& cmd, const SnappySenseData&, Stream* out) {
  sched_microtask_after(new ReadSensorsTask, 0);
  out->println("Sensor measurements gathered");
}

static void cmd_view(const String& cmd, const SnappySenseData& data, Stream* out) {
  out->println("Measurement Data");
  out->println("----------------");
  for ( SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++ ) {
    // This ignores m->display
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
      char buf[32];
      m->format(data, buf, buf+sizeof(buf));
      out->printf("%s: %s\n", m->json_key, buf);
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
  out->printf("Web server is enabled, inet address %s\n", local_ip_address());
#endif
}

Command commands[] = {
  {"hello",    "Echo the argument",                                  cmd_hello},
  {"help",     "Print help text",                                    cmd_help},
  {"scani2c",  "Scan the I2C address space and report what we find", cmd_scani2c},
  {"poweron",  "Turn on peripheral power, required for I2C",         cmd_poweron},
  {"poweroff", "Turn off peripheral power, required for I2C",        cmd_poweroff},
  {"read",     "Read the sensors",                                   cmd_read},
  {"get",      "Get the sensor reading for a specific sensor name",  cmd_get},
  {"view",     "View all the current sensor readings",               cmd_view},
  {"inet",     "Internet connectivity details",                      cmd_inet},
  {nullptr,    nullptr,                                              nullptr}
};

#endif // SERIAL_SERVER || WEB_SERVER
