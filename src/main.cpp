// Snappysense setup and main loop

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

  // Load config from NVRAM or flash, if available, otherwise use
  // default values.
  read_configuration();

#ifdef INTERACTIVE_CONFIGURATION
  if (do_interactive_configuration) {
    // Device code has powered on the serial line and the display
    render_text("Configuration mode");
    interactive_configuration(&Serial);
    // FIXME: Power down display if not in demo mode.  Although ideally
    // the device would have reset itself after config so it should not
    // be an issue, we should not come here.
  }
#endif

#ifdef DEMO_MODE
  // TODO: Alternatively, in non-demo mode, power on the display, show
  // the splash, and then power down the display again.
  show_splash();
#endif
#ifdef TIMESTAMP
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
  delay(run_scheduler(&snappy));
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
  sched_microtask_periodically(new ReadSensorsTask, sensor_poll_frequency_seconds() * 1000);
#ifdef SERIAL_SERVER
  sched_microtask_periodically(new ReadSerialInputTask, serial_command_poll_seconds() * 1000);
#endif
#ifdef MQTT_UPLOAD
  sched_microtask_after(new StartMqttTask, 0);
  sched_microtask_after(new MqttCommsTask, 0);
  sched_microtask_periodically(new CaptureSensorsForMqttTask, mqtt_capture_frequency_seconds() * 1000);
#endif
#ifdef WEB_UPLOAD
  sched_microtask_periodically(new WebUploadTask, web_upload_frequency_seconds() * 1000);
#endif
#ifdef WEB_SERVER
  sched_microtask_periodically(new ReadWebInputTask, web_command_poll_seconds() * 1000);
#endif
#ifdef DEMO_MODE
  sched_microtask_periodically(new NextViewTask, display_update_frequency_seconds() * 1000);
#endif // DEMO_MODE
}
