#include "config_ui.h"
#include "config.h"
#include "device.h"

#ifdef INTERACTIVE_CONFIGURATION

// Configuration file format version, see the 'config' statement help text further down.
// This is intended to be used in a proper "semantic versioning" way.
//
// Version 1.1:
//   Added web-config-access-point

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUGFIX_VERSION 0

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
