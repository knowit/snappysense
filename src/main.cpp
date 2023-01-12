// Snappysense setup and main loop

/////////////////////////////////////////////////////////////////////////////////////////////
//
// MODES and CONFIGURATION.
//
// SnappySense can be compile-time configured to several modes, in main.h.  These are:
//
//  - DEMO_MODE, in which the device reads the sensors often, keeps the display on,
//    and displays the sensor variables on the display in a never-ending loop.
//    Demo mode is power-hungry.
//  - !DEMO_MODE, in which the device reads the sensors much less often and turns
//    off the display and the peripherals when they are not needed.  This mode
//    conserves power (relatively).
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
//
//
// SYSTEM ARCHITECTURE.
//
// The system is constructed around a simple microtask system that currently sits on top
// of the Arduino runloop.  Periodic tasks handle polling, sensor reading, and data
// uploading; ad-hoc tasks are created to deal with system management needs.
//
// The system will sleep when there's no work to be done.  If the time to sleep is long
// enough, it will power down peripherals while it's sleeping.

#include "config.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "microtask.h"
#include "mqtt_upload.h"
#include "sensor.h"
#include "serial_server.h"
#include "snappytime.h"
#include "web_server.h"
#include "web_upload.h"

#ifdef DEMO_MODE
void show_next_view();
#endif

// Currently only one copy of sensor data globally but the code's properly parameterized and
// there could be several of these, useful in a threaded world or when snapshots of the data
// are useful.
static SnappySenseData snappy;

// Defined below all the task types
static void create_initial_tasks();

void setup() {
  bool do_interactive_configuration = false;
  device_setup(&do_interactive_configuration);
  log("SnappySense ready!\n");

  // Serial port and display are enabled now.

  // Always show the splash on startup.
  show_splash();
  delay(1000);

  // Load config from nonvolatile memory, if available, otherwise use
  // default values.
  read_configuration();

#ifdef INTERACTIVE_CONFIGURATION
  if (do_interactive_configuration) {
    render_text("Configuration mode");
    interactive_configuration(&Serial);
    enter_end_state("Press reset button!");
  }
#endif

  // We are up.
#ifdef TIMESTAMP
  // TODO: Issue 15: Is this perhaps a task?  If it fails (b/c no wifi), it should be repeated
  // until it works, but it should not block other things from happening I think.
  configure_time();
#endif
  create_initial_tasks();
  log("SnappySense running!\n");
}

// The system is constructed around a set of microtasks layered on top of the
// Arduino runloop.  See microtask.h for more documentation.

void loop() {
#ifdef TEST_MEMS
  test_mems();
#else
  unsigned long wait_time_ms = run_scheduler(&snappy);
  bool power_down = wait_time_ms >= 60*1000;
  if (power_down) {
    log("Power down: %d seconds to wait\n", wait_time_ms / 1000);
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

#ifdef DEMO_MODE
class NextViewTask final : public MicroTask {
  // -1 is the splash screen; values 0..whatever refer to the entries in the 
  // SnappyMetaData array.
  int next_view = -1;
public:
  const char* name() override {
    return "Next view";
  }
  virtual bool only_when_device_enabled() {
    return true;
  }
  void execute(SnappySenseData* data) override;
};

void NextViewTask::execute(SnappySenseData* data) {
  bool done = false;
  while (!done) {
    if (next_view == -1) {
      show_splash();
      done = true;
    } else if (snappy_metadata[next_view].json_key == nullptr) {
      // At end, wrap around
      next_view = -1;
    } else if (snappy_metadata[next_view].display == nullptr) {
      // Field not for demo_mode display
      next_view++;
    } else {
      char buf[32];
      snappy_metadata[next_view].display(*data, buf, buf+sizeof(buf));
      render_oled_view(snappy_metadata[next_view].icon, buf, snappy_metadata[next_view].display_unit);
      done = true;
    }
  }
  next_view++;
}
#endif // DEMO_MODE

static void create_initial_tasks() {
  // TODO: Issue 9: This works for most sensors but not for PIR.  We don't want to
  // poll as often as PIR needs us to (except in demo mode), so PIR needs to become
  // interrupt driven.
  sched_microtask_periodically(new ReadSensorsTask, sensor_poll_interval_s() * 1000);
#ifdef SERIAL_SERVER
  sched_microtask_periodically(new ReadSerialInputTask, serial_command_poll_interval_s() * 1000);
#endif
#ifdef MQTT_UPLOAD
  sched_microtask_after(new StartMqttTask, 0);
  sched_microtask_after(new MqttCommsTask, 0);
  sched_microtask_periodically(new CaptureSensorsForMqttTask, mqtt_capture_interval_s() * 1000);
#endif
#ifdef WEB_UPLOAD
  sched_microtask_periodically(new WebUploadTask, web_upload_interval_s() * 1000);
#endif
#ifdef WEB_SERVER
  sched_microtask_periodically(new ReadWebInputTask, web_command_poll_interval_s() * 1000);
#endif
#ifdef DEMO_MODE
  sched_microtask_periodically(new NextViewTask, display_update_interval_s() * 1000);
#endif // DEMO_MODE
}
