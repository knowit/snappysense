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

#include "command.h"
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

// FIXME - comment is wrong, some code sees this global.
// Currently only one copy of sensor data globally but the code's properly parameterized and
// there could be several of these, useful in a threaded world or when snapshots of the data
// are useful.
SnappySenseData snappy;

// Defined below all the task types
static void create_initial_tasks();

static void scheduler_loop(void*);
static TaskHandle_t scheduler_handle;

// The "big lock" temporarily avoids reentrancy problems among the tasks, but it is
// not a sensible long-term solution.  The problem seems to be that some operations,
// probably WiFi, "complete" operations by forking off tasks, allowing the task that
// uses WiFi to race ahead and start another task in the microtask scheduler (at least),
// which leads to crashes.  This needs a deeper analysis.

StaticSemaphore_t mutex_buf;
SemaphoreHandle_t big_lock = xSemaphoreCreateMutexStatic(&mutex_buf);

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
  if (xTaskCreateUniversal(scheduler_loop,
                           "snappy-sched",
                           4096,
                           nullptr,
                           1,
                           &scheduler_handle,
                           ARDUINO_RUNNING_CORE) != pdPASS) {
    enter_end_state("scheduler", true);
  };
  log("SnappySense running!\n");
  /* Not obvious that we should do this - should not the Arduino framework take care of it? */
  vTaskStartScheduler();
  /*NOTREACHED*/
}

// A scheduler task for the tasks that use the old microtask system.  This will disappear.

// TODO: With FreeRTOS tasks, how can we expect to power down peripherals if we don't know
// how tasks are sleeping?  We need some kind of callback from the scheduler for that, or
// tasks that access the
// peripherals must coordinate amongst themselves.  They can record the wait time when they go
// to sleep in a global but this is not enough storage.  Essentially, it needs to become a
// queue of wait times.
//
// It's no different with deep-sleep mode.  We must *know* what the future system demands are.

static void scheduler_loop(void*) {
  log("Entering scheduler\n");
  for(;;) {
    xSemaphoreTake(big_lock, portMAX_DELAY);
    unsigned long wait_time_ms = run_scheduler(&snappy);
    xSemaphoreGive(big_lock);
    bool power_down = wait_time_ms >= 60*1000;
    if (power_down) {
      log("Power down: %d seconds to wait\n", wait_time_ms / 1000);
      power_peripherals_off();
    }
    log("Scheduler blocking %u", wait_time_ms);
    delay(wait_time_ms);
    log("Scheduler woke\n");
    if (power_down) {
      log("Power up\n");
      power_peripherals_on();
      show_splash();
    }
  }
}

void loop() {
  // TODO: This should not be called, actually?
  // If this is not sleeping then it's sucking all the CPU out of the room.
  delay(100000);
}

#ifdef DEMO_MODE
// FreeRTOS task that requests a display update periodically to show a rotating display
// of sensor readings.
//
// Synchronization:
//  - this needs to worry about concurrent access to the data object
//  - this may need to worry about overwriting error messages on the display

static TaskHandle_t slideshow_task_handle;

static void slideshow_task(void* parameter /* const SnappySenseData* */) {
  auto* data = reinterpret_cast<const SnappySenseData*>(parameter);
  int next_view = -1;
  for(;;) {
    if (device_enabled()) {
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
    delay(display_update_interval_s() * 1000);
  }
}
#endif // DEMO_MODE

static void create_initial_tasks() {
  if (xTaskCreateUniversal(sensor_reader_task,
                           "sensor-read",
                           /* stackDepthInWords= */ 1024,
                           &snappy,
                           /* priority= */ 1,
                           &sensor_read_task_handle,
                           ARDUINO_RUNNING_CORE) != pdPASS) {
    enter_end_state("Sensor task", true);
  }
#ifdef SERIAL_SERVER
  // There should only ever be one task that reads from the serial input.
  if (xTaskCreateUniversal(serial_input_reader_task,
                           "rdln-serialsrv",
                           /* stackDepthInWords= */ 1024, 
                           new CommandHandler(/* output_stream= */ &Serial), 
                           /* priority= */ 1, 
                           &serial_input_task_handle,
                           ARDUINO_RUNNING_CORE) != pdPASS) {
    enter_end_state("Serial server task", true);
  };
#endif
#ifdef DEMO_MODE
  if (xTaskCreateUniversal(slideshow_task,
                              "slideshow",
                              /* stackDepthInWords= */ 1024, 
                              &snappy, 
                              /* priority= */ 1, 
                              &slideshow_task_handle,
                              ARDUINO_RUNNING_CORE) != pdPASS) {
    enter_end_state("Slideshow task", true);
  };
#endif // DEMO_MODE
#ifdef MQTT_UPLOAD
  if (xTaskCreateUniversal(mqtt_capture_task,
                              "mqtt-capture",
                              /* stackDepthInWords= */ 1024, 
                              &snappy, 
                              /* priority= */ 1, 
                              &mqtt_capture_task_handle,
                              ARDUINO_RUNNING_CORE) != pdPASS) {
    enter_end_state("MQTT capture task", true);
  };
  sched_microtask_after(new StartMqttTask, 0);
  sched_microtask_after(new MqttCommsTask, 0);
#endif
#ifdef WEB_UPLOAD
  sched_microtask_periodically(new WebUploadTask, web_upload_interval_s() * 1000);
#endif
#ifdef WEB_SERVER
  sched_microtask_periodically(new ReadWebInputTask, web_command_poll_interval_s() * 1000);
#endif
}
