// Snappysense setup and main loop

#include "config.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "mqtt_upload.h"
#include "sensor.h"
#include "serial_server.h"
#include "snappytime.h"
#include "web_server.h"
#include "web_upload.h"

#ifdef STANDALONE
void show_next_view();
#endif

// Currently only one copy of sensor data globally but the code's properly parameterized and
// there could be several of these, useful in a threaded world perhaps.
static SnappySenseData snappy;

#ifdef STANDALONE
// start with splash screen
static int next_view = -1;
#endif

void setup() {
  device_setup();

#ifdef STANDALONE
  show_next_view();
#else
  show_splash();
#endif

  log("SnappySense ready!\n");

#ifdef TIMESTAMP
  configure_time();
#endif
#ifdef WEB_SERVER
  start_web_server();
#endif
#ifdef MQTT_UPLOAD
  start_mqtt();
#endif
}

// This is a busy-wait loop but we don't perform readings, screen updates, and socket
// listens all the time, only every so often.  So implement a simple scheduler.

void loop() {
  static unsigned long next_sensor_reading = 0;
  static unsigned long next_web_server_action = 0;
  static unsigned long next_serial_server_action = 0;
  static unsigned long next_screen_update = 0;
  static unsigned long next_web_upload_action = 0;
  static unsigned long next_mqtt_capture_action = 0;
  static unsigned long next_mqtt_comm_action = 0;

  // A simple deadline-driven scheduler.  All this waiting is pretty bogus.
  // We want timers to trigger actions, and incoming data to trigger server
  // activity.  But it's fine for now.

  // TODO: Some logic is missing here or in lower layers:
  // - there's no sense in uploading results if the sensor has not been read
  //   since the last upload
  // - the reading should be forced in STANDALONE mode since we're going
  //   to be displaying data
  // There may be some other considerations along those lines.  Whether that
  // logic should be centralized here or pushed into the config code or the
  // upload code is uncertain.

  //
  // TODO: There's a more general idea of "action" that comes into play for mqtt
  // and the command processing.  If a message is received then the "action"
  // is to unblock a task that processes that message.  The "delay" returned 
  // by the mqtt subsystem is
  // really implicitly creating another perform_mqtt_step task.  In the command
  // processor, the "read" command really creates or unblocks a sensor reading
  // task; many other commands also create similar tasks.  It's a little open
  // whether that level of abstraction is warranted at the moment but it would
  // ensure that things don't magically happen several levels down, but are exposed
  // at the top level for proper scheduling.  On the other hand, it creates a
  // responsibility for each task to enqueue itself again.
  //
  // Do not do anything about this until after we've implemented deep-sleep state,
  // as the architecture will change to accomodate that.

  // FIXME: millis() overflows after 49 days.  We could just reboot?  And if
  // we enter deep-sleep state everything may be handled differently in any case.
  unsigned long now = millis();
  unsigned long next_deadline = ULONG_MAX;

  if (now >= next_sensor_reading) {
    get_sensor_values(&snappy);
    next_sensor_reading = now + sensor_poll_frequency_seconds() * 1000;
    next_deadline = min(next_deadline, next_sensor_reading);
  }
#ifdef SERIAL_SERVER
  if (now >= next_serial_server_action) {
    maybe_handle_serial_request(&snappy);
    next_serial_server_action = now + serial_command_poll_seconds() * 1000;
    next_deadline = min(next_deadline, next_serial_server_action);
  }
#endif
#ifdef WEB_SERVER
  if (now >= next_web_server_action) {
    maybe_handle_web_request(&snappy);
    next_web_server_action = now + web_command_poll_seconds() * 1000;
    next_deadline = min(next_deadline, next_web_server_action);
  }
#endif
#ifdef STANDALONE
  if (now >= next_screen_update) {
    show_next_view();
    next_screen_update = now + display_update_frequency_seconds() * 1000;
    next_deadline = min(next_deadline, next_screen_update);
  }
#endif // STANDALONE
#ifdef WEB_UPLOAD
  if (now >= next_web_upload_action) {
    upload_results_to_http_server(snappy);
    next_web_upload_action = now + web_upload_frequency_seconds() * 1000;
    next_deadline = min(next_deadline, next_web_upload_action);
  }
#endif
#ifdef MQTT_UPLOAD
  if (now >= next_mqtt_capture_action) {
    capture_readings_for_mqtt_upload(snappy);
    next_mqtt_capture_action = now + mqtt_capture_frequency_seconds() * 1000;
    next_deadline = min(next_deadline, next_mqtt_capture_action);
  }
  if (now >= next_mqtt_comm_action) {
    unsigned long delay_s = perform_mqtt_step();
    next_mqtt_comm_action = now + delay_s * 1000;
    next_deadline = min(next_deadline, next_mqtt_comm_action);
  }
#endif

  if (next_deadline != ULONG_MAX) {
    //log("Sleeping %d\n", (int)(next_deadline - now));
    delay(next_deadline - now);
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
