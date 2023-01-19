// Snappysense setup and main loop

/////////////////////////////////////////////////////////////////////////////////////////////
//
// SYSTEM ARCHITECTURE.
//
// The firmware is based on the Arduino framework.
//
// The firmware is constructed around a simple microtask system that sits on top
// of the Arduino runloop.  Periodic tasks, created in setup(), below, handle polling,
// sensor reading, and data uploading.  Ad-hoc tasks, created various places,
// deal with system management needs and aperiodic work.  Only one task can run at
// a time, and it runs to completion before the next task is run.  No locking is
// needed at any level.  See microtask.{cpp,h} for more.
//
// The system will sleep when there's no work to be done.  If the time to sleep is long
// enough, it will power down peripherals while it's sleeping.  All of this is managed
// in loop(), below.
//
//
// LAYERS.
//
// The system layers are roughly as follows (the design is not yet clean):
//
// Device layer:
//   Microcontroller and peripheral management, in device.{cpp,h}
//   Configuration and preferences management, in config.{cpp,h}
//   Tasking system, in microtask.{cpp,h}
//   WiFi connection management, in network.{cpp,h}
//   Serial line input, in serial_input.{cpp,h}
//   Time management, in snappytime.{cpp,h}
//   Utility functions, in util.{cpp,h}
//   Logging, in log.{cpp,h}
//
// Application layer:
//   Sensor data model, in sensor.{cpp,h}
//   MQTT sensor reading upload, in mqtt_upload.{cpp,h}
//   HTTP sensor reading upload (for development), in web_upload.{cpp,h}
//   Interactive command processing, in command.{cpp,h}
//   HTTP command processing, in web_server.{cpp,h}
//
// UI layer:
//   Sensor reading display management, in slideshow.{cpp,h} and icons.{cpp,h}
//   Orchestration, in main.cpp
//   Configuration, in main.h
//
//
// MODES and CONFIGURATION.
//
// SnappySense can be compile-time configured to several modes, in main.h.  These are:
//
//  - SLIDESHOW_MODE, in which the device reads the sensors often, keeps the display on,
//    and displays the sensor variables on the display in a never-ending loop.
//    Slideshow mode is power-hungry.
//  - Monitoring mode, aka !SLIDESHOW_MODE, in which the device reads the sensors much
//    less often and turns off the display and the peripherals when they are not needed.
//    This mode conserves power (relatively).
//  - DEVELOPER mode, which can be combined with the other two modes and which
//    allows for interactivity over the serial line, configuration values that are
//    compiled into the code, more frequent activity, and other things.
//
// When the device is powered on it can also be brought up in a "provisioning" mode,
// to set configuration variables.  To do this, connect a terminal over USB, press
// and hold the "wake" button on the device, then press and release the "reset"
// button on the CPU.  The device will enter an interactive mode over the USB line.
// There is on-line help for how to use that mode.
//
// The device may be configured to fetch the current time from a time server.
// Follow breadcrumbs from the definition of TIMESTAMP in main.h for more information.
// A simple time server is in the server/ directory in the present repo.
//
// The device can be configured to upload results to an MQTT broker (typically AWS)
// or to a Web server (for development and testing), or to neither (data are displayed
// on the built-in display).  For uploading to work, networking configuration
// variables have to be set, see config.cpp and the help text for the provisioning mode.
// Also, appropriate servers will have to be running.  There is a simple server for
// http upload in the server/ directory in the present repo.
//
// For development, the device can also listen for interactive commands over the
// serial line or on an http port, see SERIAL_SERVER and WEB_SERVER in main.h.

#include "config.h"
#include "command.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "microtask.h"
#include "mqtt_upload.h"
#include "sensor.h"
#include "snappytime.h"
#include "slideshow.h"
#include "web_server.h"
#include "web_upload.h"

// Currently we have only one copy of sensor data globally but the code's properly parameterized
// and there could be several of these (useful in a threaded world or when snapshots of the data
// are useful).  Hence the `snappy` object is private to main.cpp and is passed around to
// those that update it and those that consume it.

static SnappySenseData snappy;

void setup() {
  // Power up the device.

  bool do_interactive_configuration = false;
  device_setup(&do_interactive_configuration);
  log("SnappySense ready!\n");

  // Serial port and display are up now and can be used for output.

  // Always show the splash on startup.  The delay is for aesthetic reasons - it means
  // the display will not immediately be cleared by other operations.
  show_splash();
  delay(1000);

  // Load config from nonvolatile memory, if available, otherwise use default values.
  read_configuration();

  // We are up.  Choose between config mode and normal mode.

#ifdef INTERACTIVE_CONFIGURATION
  if (do_interactive_configuration) {
    render_text("Configuration mode");
    Serial.print("*** INTERACTIVE CONFIGURATION MODE ***\n\n");
    Serial.print("Type 'help' for help.\nThere is no line editing - type carefully.\n\n");
    sched_microtask_periodically(new ReadSerialConfigInputTask, serial_input_poll_interval_s() * 1000);
    sched_microtask_periodically(new ReadWebInputTask, web_command_poll_interval_s() * 1000);
    log("Configuration is running!\n");
    return;
  }
#endif

  // Normal mode.

#ifdef TIMESTAMP
  // Configure time as soon as we can, so run this task first.  It will reschedule itself
  // (with backoff) if it fails to connect to wifi.
  sched_microtask_after(new ConfigureTimeTask, 0);
#endif

  // Configure tasks.

  // TODO: Issue 9: This task works for most sensors but not for PIR.  We don't want to
  // poll as often as PIR needs us to (except in slideshow mode), so PIR needs to become
  // interrupt driven.
  sched_microtask_periodically(new ReadSensorsTask, sensor_poll_interval_s() * 1000);
#ifdef SERIAL_SERVER
  sched_microtask_periodically(new ReadSerialCommandInputTask, serial_input_poll_interval_s() * 1000);
#endif
#ifdef MQTT_UPLOAD
  sched_microtask_after(new StartMqttTask, 0);
  sched_microtask_after(new MqttCommsTask, 0);
  sched_microtask_periodically(new CaptureSensorsForMqttTask, mqtt_capture_interval_s() * 1000);
#endif
#ifdef WEB_UPLOAD
  sched_microtask_periodically(new WebUploadTask, web_upload_interval_s() * 1000);
#endif
#ifdef WEB_COMMAND_SERVER
  sched_microtask_periodically(new ReadWebInputTask, web_command_poll_interval_s() * 1000);
#endif
#ifdef SLIDESHOW_MODE
  sched_microtask_periodically(new SlideshowTask, slideshow_update_interval_s() * 1000);
#endif // SLIDESHOW_MODE

  log("SnappySense running!\n");
}

void loop() {
#ifdef TEST_MEMS
  test_mems();
#else
  unsigned long wait_time_ms = run_scheduler(&snappy);
  bool power_down = wait_time_ms >= 60*1000;
  if (power_down) {
    log("Power down: %u seconds to wait\n", (unsigned)(wait_time_ms / 1000));
    power_peripherals_off();
  }
  delay(wait_time_ms);
  if (power_down) {
    log("Power up\n");
    power_peripherals_on();
    show_splash();
  }
#endif
}
