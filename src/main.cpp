/*
* Project: esp32-snappysense-sandbox
* Description: This application is used to verify and debug SnappySense hardware
* Author: Gunnar L. Larsen
* Date: 12/20/2022
*
* Setup
* Install the PlatformIO extension in Visual Code
*/

#include "config.h"
#include "device.h"
#include "icons.h"
#include "sensor.h"
#include "SerialCommands.h"
#include "snappytime.h"
#include "web_server.h"
#include "web_upload.h"

// Forward function declarations
void cmd_unrecognized(SerialCommands* sender, const char* cmd);
void cmd_hello(SerialCommands* sender);
void cmd_scani2c(SerialCommands* sender);
void cmd_poweron(SerialCommands* sender);
void cmd_poweroff(SerialCommands* sender);
void cmd_read(SerialCommands* sender);
void cmd_view(SerialCommands* sender);
#ifdef STANDALONE
void show_next_view();
#endif

// Serial buffer for Stream based command parser
char serial_command_buffer_[32];

// Declaration of supported commands
SerialCommand cmd_hello_("hello", cmd_hello);
SerialCommand cmd_scani2c_("scani2c", cmd_scani2c);
SerialCommand cmd_poweron_("poweron", cmd_poweron);
SerialCommand cmd_poweroff_("poweroff", cmd_poweroff);  // This doesn't work so well
SerialCommand cmd_read_("read", cmd_read);
SerialCommand cmd_view_("view", cmd_view);

// Global instance variables
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

// SnappySense specifics
int next_view = -1; /* start with splash screen */

#if defined(WEBSERVER) && !defined(STANDALONE)
#  error "WEBSERVER implies STANDALONE"
#endif

void setup() {
  device_setup();

  // Set up serial commands
  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_hello_);
  serial_commands_.AddCommand(&cmd_scani2c_);
  serial_commands_.AddCommand(&cmd_poweron_);
  serial_commands_.AddCommand(&cmd_poweroff_);
  serial_commands_.AddCommand(&cmd_read_);
  serial_commands_.AddCommand(&cmd_view_);

  // Set up ble commands
  // TODO

#ifdef STANDALONE
  show_next_view();
#else
  show_splash();
#endif
  Serial.println("SnappySense ready!");  

#ifdef TIMESTAMP
  configure_time();
#endif
#ifdef WEBSERVER
  start_web_server();
#endif
}

// This is a busy-wait loop but we don't perform readings, screen updates, and socket
// listens all the time, only every so often.  So implement a simple scheduler.
//
// TODO: We read the sensor values every time we enter, and we enter because we want
// to display or upload something.  But esp for uploads it would be sensible to
// perform readings and uploads on independent schedules, ie, we could read every
// hour (say) but upload every few hours to preserve battery.  It depends on the
// application.  It may also be that if we implement deep-sleep, none of that matters
// since the device starts up anew after sleeping and it's not sensible to persist
// data and upload later.
void loop() {
  static unsigned long next_server_action = 0;
  static unsigned long next_screen_update = 0;
  static unsigned long next_client_action = 0;

  // check USB serial for command
  serial_commands_.ReadSerial();

  // Read the sensors.
  get_sensor_values();

  // dumb scheduler
  unsigned long now = millis();
  unsigned long next_deadline = ULONG_MAX;

  // show slideshow and handle web, if enabled.  this waiting is all pretty bogus.
  // we want timers to trigger screen and client actions, and incoming data to
  // trigger server activity.
#ifdef STANDALONE
#  ifdef WEBSERVER
  if (now > next_server_action) {
    maybe_handle_web_request(snappy);
    next_server_action = now + WEBSERVER_WAIT_TIME_MS;
    next_deadline = min(next_deadline, next_server_action);
  }
#  endif
  if (now > next_screen_update) {
    show_next_view();
    next_screen_update = now + display_update_frequency_seconds() * 1000;
    next_deadline = min(next_deadline, next_screen_update);
  }
#endif // STANDALONE
#ifdef WEB_UPLOAD
  if (now > next_client_action) {
    upload_results_to_http_server(snappy);
    next_client_action = now + web_upload_frequency_seconds() * 1000;
    next_deadline = min(next_deadline, next_client_action);
  }
#endif

  if (next_deadline != ULONG_MAX) {
    delay(next_deadline - now);
  }
}

//This is the default handler, and gets called when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
  sender->GetSerial()->print("Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}

void cmd_hello(SerialCommands* sender) {
  char *arg;
  arg = sender->Next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL) {    // As long as it returns something, take it
    sender->GetSerial()->print("Hello ");
    sender->GetSerial()->println(arg);
  }
  else {
    sender->GetSerial()->println("Hello, whoever you are");
  }
}

void cmd_scani2c(SerialCommands* sender) {
  auto* out = sender->GetSerial();
  out->println("Scanning...");
  int num = probe_i2c_devices(out);
  out->print("Number of I2C devices found: ");
  out->println(num, DEC);
  return;  
}

void cmd_poweron(SerialCommands* sender) {
  power_on();
  sender->GetSerial()->println("Peripheral power turned on");
}

void cmd_poweroff(SerialCommands* sender) {
  power_off();
  sender->GetSerial()->println("Peripheral power turned off");
}

void cmd_read(SerialCommands* sender) {
  get_sensor_values();
  sender->GetSerial()->println("Sensor measurements gathered");
}

void cmd_view(SerialCommands* sender) {
  auto *s = sender->GetSerial();
  s->println("Measurement Data");
  s->println("----------------");
  for ( SnappyMetaDatum* m = snappy_metadata; m->json_key != nullptr; m++ ) {
    // This ignores m->display
    s->print(m->explanatory_text);
    s->print(": ");
    char buf[32];
    m->format(snappy, buf, buf+sizeof(buf));
    s->print(buf);
    s->print(" ");
    s->println(m->unit_text);
  }
}

#ifdef STANDALONE
void show_next_view() {
  bool done = false;
  while (!done) {
    if (next_view == -1) {
      show_splash();
      done = true;
    } else if (snappy_metadata[next_view].json_key == nullptr) {
      // At end, wrap around
      next_view = -1;
    } else if (snappy_metadata[next_view].display == nullptr) {
      // Field not for standalone display
      next_view++;
    } else {
      char buf[32];
      snappy_metadata[next_view].display(snappy, buf, buf+sizeof(buf));
      render_oled_view(snappy_metadata[next_view].icon, buf, snappy_metadata[next_view].display_unit);
      done = true;
    }
  }
  next_view++;
}
#endif // STANDALONE
