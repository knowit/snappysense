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
// The system layers are roughly as follows, from lowest to highest:
//
// OS and framework layer:
//   Arduino, FreeRTOS and HAL
//
// Device layer:
//   Microcontroller and peripheral management and clock, in device.{cpp,h}
//   Configuration and preferences management, in config.{cpp,h}
//   Utility functions, in util.{cpp,h}
//   Serial line logging, in log.{cpp,h}
//
// Tasking layer:
//   Tasking system, in microtask.{cpp,h}
//
// Connectivity layer:
//   Serial line input, in serial_input.{cpp,h}
//   WiFi connection management, in network.{cpp,h}
//   Web server infrastructure, in web_server.{cpp,h}
//   Time configuration service, in snappytime.{cpp,h}
//
// Application logic layer:
//   Sensor data model, in sensor.{cpp,h}
//   MQTT data upload and command handling, in mqtt_upload.{cpp,h}
//   HTTP data upload (for development), in web_upload.{cpp,h}
//   Serial & web interactive command processing (for development), in command.{cpp,h}
//
// Application UI layer:
//   Sensor reading display management, in slideshow.{cpp,h} and icons.{cpp,h}
//   Configuration user interface (serial and web), in config_ui.{cpp,h}
//   Orchestration of everything, in main.cpp
//   Compile-time configuration, in main.h
//
// TODO: The sensor data model is really more than one thing.  There's the model
// for sensor readings, which really belongs with the device.  And then there's the
// sensor metadata, which handle things like formatting and so on; these are part of
// the application logic and UI, properly.  (Formatting for MQTT is part of the
// application logic, formatting for display part of the UI.  But it's probably
// OK not to worry overmuch about that.)
//
// Note that in some cases, lower layers do call higher layers.  For example, the
// tasking system deletes ad-hoc tasks when they're done and have not been rescheduled.
// The task destructor is a virtual that may have been overridden by the task class,
// which is managed by a higher level.
//
//
// MODES and CONFIGURATION.
//
// SnappySense can run in one of two modes, selectable at run-time by a button press.
//
// In SLIDESHOW MODE it reads the sensors often, keeps the display on, and displays
// the sensor variables on the display in a never-ending loop.  Slideshow mode is
// power-hungry.
//
// In MONITORING MODE the device reads the sensors much less often and turns off the
// display and the peripherals when they are not needed.  It uploads data rarely.
// This mode conserves power (relatively).
//
// SnappySense can also be configured at compile time to run in DEVELOPER mode, which
// usually allows for interactivity over the serial line, configuration values that are
// compiled into the code, more frequent observation and upload activity, and other things.
//
// When the device is powered on it can also be brought up in a PROVISIONING MODE,
// to set configuration variables.  See CONFIG.md at the root for more information.
//
// The device may be configured to fetch the current time from a time server.
// Follow breadcrumbs from the definition of TIMESTAMP in main.h for more information.
// A simple time server is in the test-server/ directory in the present repo.
//
// The device can be configured to upload results to an MQTT broker (typically AWS)
// or to a Web server (for development and testing), or to neither.  For uploading to
// work, networking configuration variables have to be set, see CONFIG.md.  Also,
// appropriate servers will have to be running.  There is a simple server for http
// upload in the test-server/ directory in the present repo.
//
// For development, the device can also listen for interactive commands over the
// serial line or on an http port, see SERIAL_COMMAND_SERVER and WEB_COMMAND_SERVER
// in main.h.

#include "config.h"
#include "config_ui.h"
#include "command.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "microtask.h"
#include "mqtt_upload.h"
#include "piezo.h"
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

// The default is to start the slideshow on startup.  It can be toggled to monitoring mode by a press
// on the wake button.

#ifdef SLIDESHOW_MODE
bool slideshow_mode = true;
#else
bool slideshow_mode = false;
#endif // SLIDESHOW_MODE

QueueHandle_t/*<int>*/ main_event_queue;

static TimerHandle_t scheduler_timer;

static void sched_clock_callback(TimerHandle_t t) {
  int ev = EV_CLOCK;
  xQueueSend(main_event_queue, &ev, portMAX_DELAY);
}

void setup() {
  main_event_queue = xQueueCreate(10, sizeof(int));

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

  // We sometimes need random numbers, try to seed the stream.
  randomSeed(entropy());

  // We are up.  Choose between config mode and normal mode.

#ifdef WEB_CONFIGURATION
  if (do_interactive_configuration) {
    render_text("Configuration mode");
#ifdef WEB_CONFIGURATION
    sched_microtask_periodically(new WebConfigTask, web_command_poll_interval_s() * 1000);
    log("Web configuration is running!\n");
#endif
    return;
  }
#endif

  // Normal mode.

#ifdef TIMESERVER
  // Configure time as soon as we can, so run this task first.  It will reschedule itself
  // (with backoff) if it fails to connect to wifi.
  sched_microtask_after(new ConfigureTimeTask, 0);
#endif

  // Configure tasks.

  // TODO: Issue 9 / Issue 33: This task works for most sensors but not for
  // noise and motion.
  sched_microtask_periodically(new ReadSensorsTask, sensor_poll_interval_s() * 1000);
#ifdef SERIAL_COMMAND_SERVER
  sched_microtask_periodically(new SerialCommandTask, serial_input_poll_interval_s() * 1000);
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
  sched_microtask_periodically(new WebCommandTask, web_command_poll_interval_s() * 1000);
#endif
  if (slideshow_mode) {
    sched_microtask_periodically(new SlideshowTask, slideshow_update_interval_s() * 1000);
  }

  scheduler_timer = xTimerCreate("sched", pdMS_TO_TICKS(100), pdFALSE, NULL, sched_clock_callback);

  log("SnappySense running!\n");
#if defined(SNAPPY_PIEZO)
# if defined(STARTUP_SONG)
  static const char melody[] = "StarWars:d=4,o=5,b=38:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6";
# else
  static const char melody[] = "Beep:d=4,o=5,b=38:16p,16c";
# endif
  play_song(melody);
#endif
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

  // Wait until our delay expires or a button press arrives.
  xTimerChangePeriod(scheduler_timer, pdMS_TO_TICKS(wait_time_ms), portMAX_DELAY);
again:
  int ev;
  while (xQueueReceive(main_event_queue, &ev, portMAX_DELAY) != pdTRUE) {}
  switch (ev) {
  case EV_CLOCK:
    break;
  case EV_BUTTON_DOWN:
    log("Button down\n");
    goto again;
  case EV_BUTTON_UP:
    log("Button up\n");
    goto again;
  default:
    panic("Unknown event");
  }

  if (power_down) {
    log("Power up\n");
    power_peripherals_on();
    show_splash();
  }
#endif
}
