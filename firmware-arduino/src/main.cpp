// Snappysense setup and main loop

/////////////////////////////////////////////////////////////////////////////////////////////
//
// SYSTEM ARCHITECTURE.
//
// The firmware is based on the Arduino framework.
//
// The firmware is constructed around an event queue processed by the Arduino runloop.  There
// is one global event queue, and many timers feeding it.  See below for a better description.
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
//   Microcontroller and peripheral management, in device.{cpp,h}
//   Configuration and preferences management, in config.{cpp,h}
//   Utility functions, in util.{cpp,h}
//
// Event queue management
//   In main.{h,cpp}
//
// Connectivity and user interaction layer:
//   Serial line polling, in serial_server.{cpp,h}
//   WiFi connection management, in network.{cpp,h}
//   Web server infrastructure, in web_server.{cpp,h}
//   Time configuration service, in time_server.{cpp,h}
//   Serial line logging, in log.{cpp,h}
//   Button logic, in button.{cpp,h}
//
// Application logic layer:
//   Sensor data model, in sensor.{cpp,h}
//   MQTT data upload and command handling, in mqtt.{cpp,h}
//   Serial line interactive command processing (for development), in command.{cpp,h}
//
// Application UI layer:
//   Sensor reading display management, in slideshow.{cpp,h} and icons.{cpp,h}
//   Configuration web user interface, in web_config.{cpp,h}
//   Orchestration of everything, in main.cpp
//   Compile-time configuration, in main.h
//
// The sensor data model is really more than one thing...  There's the model
// for sensor readings, which really belongs with the device.  And then there's the
// sensor metadata, which handle things like formatting and so on; these are part of
// the application logic and UI, properly.  (Formatting for MQTT is part of the
// application logic, formatting for display part of the UI.  But it's probably
// OK not to worry overmuch about that.)
//
// Generally speaking, lower levels never call to higher levels.
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
// SnappySense can also be configured at compile time to run in SNAPPY_DEVELOPMENT mode, which
// usually allows for interactivity over the serial line, configuration values that are
// compiled into the code, more frequent observation and upload activity, and other things.
//
// When the device is powered on it can also be brought up in a PROVISIONING MODE,
// to set configuration variables.  See CONFIG.md at the root for more information.
//
// The device may be configured to fetch the current time from a time server.
// Follow breadcrumbs from the definition of SNAPPY_NTP in main.h for more information.
//
// The device can be configured to upload results to an MQTT broker (typically AWS).
// For uploading to work, networking configuration variables have to be set, see CONFIG.md.
// Also,an  appropriate server will have to be running.  At the moment, only AWS IoT MQTT
// is supported, this should be fixed.
//
// For development, the device can also listen for interactive commands over the
// serial line, see SERIAL_COMMAND_SERVER in main.h.
//
//
// STATE MACHINE
//
// In normal mode there are conceptually many concurrent tasks running:
//
// The main loop
// The monitoring / observation task
// The display / slideshow task
// The short button press listener
// The long button press listener
// The wifi manager task
// The serial port listener (if the serial port command processor is present)
// The mqtt task (download + upload)
// The web upload task (download + upload)
// The music player task
// (And more, depending on configuration)
//
// All but the music player are integrated into the main loop, driven by several timers.
// The music player is an actual FreeRTOS concurrent task with a higher priority.  They could all
// have been different FreeRTOS tasks but this would not have been a simplification, as we want
// to be able to react to button presses and other interrupts and that means controlling the
// other tasks from the main task; also, with other tasks come issues of concurrency and mutual
// exclusion.
//
// The MAIN LOOP goes through a number of states as follows:
//
//        Boot
//
//        Start slideshow task
//        Start button listeners
//        Start serial port listener, if applicable
//
//    L1  Open the communications window
//          bring up wifi
//          configure time, if not already done
//          unless upload is disabled
//            communicate with mqtt or web server, depending on what's selected
//            this means polling; ticks on polling timer cause callbacks to network code
//          set the comms timeout
//          while the timeout not expired
//            await notifications about comms activity
//            reset the comms timeout every time there is activity
//
//        Close the communication window
//          bring down wifi
//
//    L2  Play the slideshow for a little while
//
//    L3  If device is in monitoring mode & we don't have a serial listener, go to sleep for a while
//          Power down
//          Halt the slideshow
//          If while sleeping, the button is pressed
//            Wake up
//            Flash "Monitoring mode" on the screen for a couple seconds
//            Go to L2 (a second press during L2 will bring us into slideshow mode,
//              see button tasks below)
//        Otherwise
//          Let the slide show play for a while without doing anything else
//
//    L4  If in monitoring mode, Wake up (otherwise we're awake)
//          Power up
//          Start slideshow task
//
//        Open the monitoring window
//          (takes a while to monitor)
//        Close the monitoring window
//
//        Monitoring data arrive, they are distributed to slideshow and comms
//
//        Goto L1
//
// The SHORT-PRESS BUTTON LISTENER task:
//
//    The buttons are interrupt-driven.  Button press and release are delivered to the main loop
//    and decoded.  The main loop calls short/long press handlers when appropriate.
//
// The SERIAL LINE LISTENER task:
//
//    This is a polling listener that runs often, keeps the device awake, and reads input from
//    the serial port.  When there is some input, it is handled in a manner TBD.
//
// The SOCKET LISTENER FOR MQTT and SOCKET LISTENER FOR WEB UPLOAD tasks:
//
//    These are active only during the comms window.
//
// The SOCKET LISTENER FOR WEB SERVER task:
//
//    This should probably go away, but it's like the serial port listener: it keeps the device
//    on and listens actively.

#include "button.h"
#include "config.h"
#include "command.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "mqtt.h"
#include "network.h"
#include "piezo.h"
#include "sensor.h"
#include "serial_server.h"
#include "slideshow.h"
#include "time_server.h"
#include "web_config.h"
#include "web_server.h"

// The default is to start the slideshow on startup.  It can be toggled to monitoring
// mode by a press on the wake button.  Slideshow mode really only affects what we do
// after communication and before monitoring, and for how long.
bool slideshow_mode = true;

// The timer used for timing out the major sections of the main loop.
static TimerHandle_t master_timeout_timer;

// The event queue that drives all activity except within the music player task
static QueueHandle_t/*<SnappyEvent>*/ main_event_queue;

void setup() {
  main_event_queue = xQueueCreate(100, sizeof(SnappyEvent));

  // Power up the device.
  device_setup();
  // Serial port and display are up now and can be used for output.

  // Load config from nonvolatile memory, if available, otherwise use default values.
  read_configuration();

  // We sometimes need random numbers, try to seed the stream.
  randomSeed(entropy());

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

void put_main_event(EvCode code) {
  // This (and the ones below) can be called from timer callbacks, so use a delay of 0.
  // The queue should anyway be large enough for us never to have to block on insert.
  SnappyEvent ev(code);
  xQueueSend(main_event_queue, &ev, 0);
}

void put_main_event(EvCode code, void* data) {
  SnappyEvent ev(code, data);
  xQueueSend(main_event_queue, &ev, 0);
}

void put_main_event(EvCode code, uint32_t data) {
  SnappyEvent ev(code, data);
  xQueueSend(main_event_queue, &ev, 0);
}

void put_main_event_from_isr(EvCode code) {
  SnappyEvent ev(code);
  xQueueSendFromISR(main_event_queue, &ev, nullptr);
}

static void init_master_timeout() {
  master_timeout_timer = xTimerCreate("main",
                                      1,
                                      pdFALSE,
                                      nullptr,
                                      [](TimerHandle_t t) {
                                        put_main_event(EvCode((uint32_t)pvTimerGetTimerID(t)));
                                      });
}

static void set_master_timeout(unsigned timeout_ms, EvCode payload) {
  vTimerSetTimerID(master_timeout_timer, (void*)payload);
  xTimerChangePeriod(master_timeout_timer, pdMS_TO_TICKS(timeout_ms), portMAX_DELAY);
}

static void cancel_master_timeout() {
  xTimerStop(master_timeout_timer, portMAX_DELAY);
}

#ifdef SNAPPY_WEBCONFIG
static void ap_mode_loop() NO_RETURN;
#endif

// This never returns.  The Arduino main loop does nothing interesting for us.  Our main loop
// is based on FreeRTOS and is not busy-waiting.

void loop() {
  /////////////////////////////////////////////////////////////////////////////////////////
  //
  // Main task's state

#ifdef SNAPPY_WIFI
  // True iff the main loop is in the communication window, between COMM_WIFI_CLIENT_UP
  // and COMM_ACTIVITY_EXPIRED.
  bool in_communication_window = false;

  // True iff the main loop is in the wifi window, between COMM_START and COMM_ACTIVITY_EXPIRED.
  bool in_wifi_window = false;
#endif

  // True iff the main loop is in the monitoring window, between MONITOR_START and MONITOR_STOP.
  bool in_monitoring_window = false;

  // True when the peripherals have been powered down and we are between SLEEP_START and
  // POST_SLEEP.
  bool in_sleep_window = false;

  // This starts out the same as slideshow_mode but can be changed by a button press,
  // and is acted upon at a specific point in the state machine
  bool slideshow_next_mode = slideshow_mode;

#ifdef SNAPPY_COMMAND_PROCESSOR
  // Data held for the command processor.
  SnappySenseData command_data;
#endif

  /////////////////////////////////////////////////////////////////////////////////////////
  //
  // Slideshow / display task's state state

  init_master_timeout();

  // The slideshow task starts whether we're in slideshow mode or not, since slideshow
  // mode only affects what happens between communication and monitoring, and for how long.
  // The first thing the task does is show the splash screen.
  put_main_event(EvCode::SLIDESHOW_START);

  // Start the main task.
  put_main_event(EvCode::START_CYCLE);

  // Start concurrent tasks.
#ifdef SNAPPY_WIFI
  wifi_init();
#endif
#ifdef SNAPPY_NTP
  timeserver_init();
#endif
#ifdef SNAPPY_MQTT
  mqtt_init();
#endif
#ifdef SNAPPY_SERIAL_INPUT
  serial_server_init();
  serial_server_start();
#endif
  monitoring_init();
  button_init();
  slideshow_init();

  // Shut up the compiler
  (void)in_monitoring_window;

  // This is used to improve the UX.  It shortens the comm window the first time around and
  // skips the relaxation / sleep before we read the sensors.
  bool first_time = true;

  for (;;) {
    SnappyEvent ev;
    while (xQueueReceive(main_event_queue, &ev, portMAX_DELAY) != pdTRUE) {}
    //log("Event %d\n", (int)ev.code);
    switch (ev.code) {

      /////////////////////////////////////////////////////////////////////////////////////////////
      //
      // Main task

      case EvCode::START_CYCLE: {
        // We communicate only if we have to.  Note that the predicates for comm work have the
        // ability to reduce the frequency of communication if they want; for example, the
        // mqtt task could decide not to communicate often, or if it doesn't have a lot of
        // data to upload.
#ifdef SNAPPY_WIFI
        bool comm_work = false;
#ifdef SNAPPY_NTP
        comm_work = comm_work || timeserver_have_work();
#endif
#ifdef SNAPPY_MQTT
        comm_work = comm_work || mqtt_have_work();
#endif
        if (comm_work) {
          put_main_event(EvCode::COMM_START);
        } else {
          put_main_event(EvCode::POST_COMM);
        }
#else
        put_main_event(EvCode::SLEEP_START);
#endif
        break;
      }

#ifdef SNAPPY_WIFI
      case EvCode::COMM_START:
        // Open the communication window: bring up the WiFi client.  This will respond with either
        // COMM_WIFI_CLIENT_UP or COMM_WIFI_CLIENT_FAILED.
        wifi_enable_start();
        in_wifi_window = true;
        break;

      case EvCode::COMM_WIFI_CLIENT_RETRY:
        // As above, but retrying after a timeout.
        wifi_enable_retry();
        break;

      case EvCode::COMM_WIFI_CLIENT_FAILED:
        // Could not bring up WiFi.
        put_main_event(EvCode::MESSAGE, new String("No WiFi"));
        in_wifi_window = false;
        put_main_event(EvCode::POST_COMM);
        break;

      case EvCode::COMM_WIFI_CLIENT_UP: {
        in_communication_window = true;
#ifdef SNAPPY_NTP
        if (timeserver_have_work()) {
          timeserver_start();
        }
#endif
#ifdef SNAPPY_MQTT
        if (mqtt_have_work()) {
          mqtt_start();
        }
#endif
        unsigned long timeout_ms = comm_activity_timeout_s() * 1000;
        if (first_time) {
          timeout_ms /= 2;
        }
        set_master_timeout(timeout_ms, EvCode::COMM_ACTIVITY_EXPIRED);
        break;
      }

      case EvCode::COMM_ACTIVITY:
        // Some component had WiFi activity, so keep the WiFi up a while longer, unless this
        // message arrives late.
        if (in_communication_window) {
          unsigned long timeout_ms = comm_activity_timeout_s() * 1000;
          if (first_time) {
            timeout_ms /= 2;
          }
          set_master_timeout(timeout_ms, EvCode::COMM_ACTIVITY_EXPIRED);
        }
        break;

      case EvCode::COMM_ACTIVITY_EXPIRED:
        // No WiFi activity for a while, so bring down WiFi and move to the next phase.
        // This was a timer message, which could be received after the comm window has closed.
        // If so, we should not put a POST_COMM1 event.
        if (in_communication_window || in_wifi_window) {
          put_main_event(EvCode::POST_COMM);
        }
        if (in_communication_window) {
#ifdef SNAPPY_MQTT
          mqtt_stop();
#endif
#ifdef SNAPPY_NTP
          timeserver_stop();
#endif
          in_communication_window = false;
        }
        if (in_wifi_window) {
          wifi_disable();
          in_wifi_window = false;
        }
        break;

      case EvCode::POST_COMM:
        // The process that started in COMM_START task always ends up here, via some of the states
        // above.
        //
        // The comm window is closed.  Let the slideshow continue for a bit before deciding
        // what mode we're going to be in.
        //
        // TODO: In an ideal world, the commencement of sleep would coincide with the end of the
        // slidehow cycle.  We could implement this by having SLEEP_START set a flag and have
        // a message from the slideshow at the end of the cycle see that flag and actually
        // enter sleep mode.  But this adds more complexity.
        assert(!in_communication_window && !in_wifi_window);
        if (first_time) {
          put_main_event(EvCode::SLEEP_START);
        } else {
          set_master_timeout(comm_relaxation_timeout_s() * 1000, EvCode::SLEEP_START);
        }
        break;
#endif

      case EvCode::SLEEP_START:
        // Figure out what mode we're in.  In monitoring mode, we turn off the screen and go
        // into low-power state.  In slideshow mode, we continue on as we were, for a while.
        if (first_time) {
          put_main_event(EvCode::POST_SLEEP);
        } else {
          slideshow_mode = slideshow_next_mode;
          log("New mode: %s\n", slideshow_mode ? "slideshow" : "monitoring");
          if (slideshow_mode) {
            set_master_timeout(slideshow_mode_sleep_s() * 1000, EvCode::POST_SLEEP);
          } else {
            put_main_event(EvCode::SLIDESHOW_STOP);
            set_master_timeout(monitoring_mode_sleep_s() * 1000, EvCode::POST_SLEEP);
            log("Nap time.  Sleep mode activated.\n");
            power_peripherals_off();
            in_sleep_window = true;
          }
        }
        break;

      case EvCode::POST_SLEEP:
        if (in_sleep_window) {
          // We can come to POST_SLEEP from either the timeout or from a button press.
          cancel_master_timeout();
          power_peripherals_on();
          log("Is anyone there?\n");
          in_sleep_window = false;
          put_main_event(EvCode::SLIDESHOW_RESET);
          put_main_event(EvCode::SLIDESHOW_START);
        }
        put_main_event(EvCode::MONITOR_START);
        first_time = false;
        break;

      case EvCode::MONITOR_START:
        log("Monitoring window opens\n");
        in_monitoring_window = true;
        monitoring_start();
        set_master_timeout(monitoring_window_s() * 1000, EvCode::MONITOR_STOP);
        break;

      case EvCode::MONITOR_STOP:
        monitoring_stop();
        put_main_event(EvCode::START_CYCLE);
        log("Monitoring window closes\n");
        in_monitoring_window = false;
        break;

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Events delivered to the main task

      case EvCode::MONITOR_DATA: {
        log("Monitor data received\n");
        // monitor data arrived after closing the monitoring window
        SnappySenseData* new_data = (SnappySenseData*)ev.pointer_data;
        assert(new_data != nullptr);
#ifdef SNAPPY_MQTT
        mqtt_add_data(new SnappySenseData(*new_data));
#endif
#ifdef SNAPPY_COMMAND_PROCESSOR
        command_data = *new_data;
#endif
        // slideshow_new_data takes over the ownership of the new_data.
        slideshow_new_data(new_data);
        break;
      }

      case EvCode::BUTTON_PRESS:
        if (in_sleep_window) {
          // Wake up and move the state machine along.  POST_SLEEP will cancel any pending timeout.
          put_main_event(EvCode::POST_SLEEP);
          put_main_event(EvCode::MESSAGE, new String(slideshow_mode ? "Slideshow mode" : "Monitoring mode"));
          break;
        }

        slideshow_next_mode = !slideshow_next_mode;
        put_main_event(EvCode::SLIDESHOW_RESET);
        put_main_event(EvCode::MESSAGE, new String(slideshow_next_mode ? "Slideshow mode" : "Monitoring mode"));
        put_main_event(EvCode::SLIDESHOW_START);
        break;

      case EvCode::ENABLE_DEVICE:
        set_device_enabled(true);
        break;

      case EvCode::DISABLE_DEVICE:
        set_device_enabled(false);
        break;

      case EvCode::SET_INTERVAL:
        set_mqtt_capture_interval_s(ev.scalar_data);
        break;

#ifdef SNAPPY_COMMAND_PROCESSOR
      case EvCode::PERFORM: {
        String* cmd = (String*)ev.pointer_data;
        command_evaluate(*cmd, command_data, Serial);
        delete cmd;
        break;
      }
#endif

#ifdef SNAPPY_WEBCONFIG
      case EvCode::BUTTON_LONG_PRESS:
        // Major mode change.  This is special: it knows a bit too much about the rest of the
        // state machine but that's just how it's going to be.  We're trying to avoid having
        // to process any other messages before switching to a completely different mode.
        // We never switch back: we restart the device when AP mode ends.

        // We're in the end times.
        cancel_master_timeout();
        slideshow_stop();

        // We need the screen, so power up i2c if we're powered down.
        if (in_sleep_window) {
          log("Powered up for AP mode\n");
          power_peripherals_on();
          in_sleep_window = false;
        }

        // Stop monitoring if we're doing that.  We may get a callback about new data
        // but this is benign, AP mode will discard it.
        if (in_monitoring_window) {
          // This must stop all timers in the monitoring code
          monitoring_stop();
          in_monitoring_window = false;
        }

        // Shut down communication.  This may leave things unsent and time unconfigured
        // but nobody really cares about a little lost data.
        if (in_communication_window) {
          // This must stop all timers in the communication code
#ifdef SNAPPY_MQTT
          mqtt_stop();
#endif
#ifdef SNAPPY_NTP
          timeserver_stop();
#endif
          in_communication_window = false;
        }

        // Power down wifi, we're going to enter AP mode shortly.
        if (in_wifi_window) {
          wifi_disable();
          in_wifi_window = false;
        }

        // AP mode is not interactive.
#ifdef SNAPPY_SERIAL_INPUT
        serial_server_stop();
#endif

        // ap_mode_loop() never returns, it restarts the device.
        ap_mode_loop();
#endif // SNAPPY_WEBCONFIG

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Monitor task

      case EvCode::MONITOR_WORK:
        monitoring_tick(ev.scalar_data);
        break;

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Communication task

#ifdef SNAPPY_MQTT
      case EvCode::COMM_MQTT_WORK:
        mqtt_work();
        break;
#endif
#ifdef SNAPPY_NTP
      case EvCode::COMM_NTP_WORK:
        timeserver_work();
        break;
#endif

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Display and slideshow task.  The RESET/START/STOP events are here to ensure that
      // operations are performed in the correct order.  Code that posts them can be sure
      // that they are executed after any operations that may already be in the queue.

      case EvCode::MESSAGE:
        slideshow_show_message_once((String*)ev.pointer_data);
        break;

      case EvCode::SLIDESHOW_RESET:
        slideshow_reset();
        break;

      case EvCode::SLIDESHOW_START:
        slideshow_start();
        break;

      case EvCode::SLIDESHOW_STOP:
        slideshow_stop();
        break;

      case EvCode::SLIDESHOW_WORK:
        slideshow_next();
        break;

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Button monitor task

      case EvCode::BUTTON_DOWN:
        button_down();
        break;

      case EvCode::BUTTON_UP:
        button_up();
        break;

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Serial monitor task

#ifdef SNAPPY_SERIAL_INPUT
      case EvCode::SERIAL_SERVER_POLL:
        serial_server_poll();
        break;
#endif

      default:
        // WEB_POLL, WEB_REQUEST, WEB_REQUEST_FAILED are used only in AP mode
        panic("Unknown event");
    }
  }
}

#ifdef SNAPPY_WEBCONFIG

// Event processing loop that never returns.  Processes events from the main queue, but
// ignores most of them.  In particular, the serial line commands are not available in
// this mode.

static void ap_mode_loop() {
  // We could drain the event queue here as everything that's pending is bogus,
  // but it seems safe for now to discard unknow events in the loop below instead.

  // Create an access point and advertise it on the screen
  if (!webcfg_start_access_point()) {
    panic("Access point failed");
  }

  // Create a web server on the access point
  web_server_init(80);
  web_server_start();

  // Handle web + buttons until we reboot
  for (;;) {
    SnappyEvent ev;
    while (xQueueReceive(main_event_queue, &ev, portMAX_DELAY) != pdTRUE) {}
    //log("AP event %d\n", (int)ev.code);
    switch (ev.code) {

      case EvCode::WEB_SERVER_POLL:
        web_server_poll();
        break;

      case EvCode::WEB_REQUEST: {
        WebRequest* r = (WebRequest*)ev.pointer_data;
        webcfg_process_request(r->client, r->request);
        web_server_request_completed(r);
        break;
      }

      case EvCode::WEB_REQUEST_FAILED: {
        WebRequest* r = (WebRequest*)ev.pointer_data;
        webcfg_failed_request(r->client, r->request);
        web_server_request_completed(r);
        break;
      }

      case EvCode::BUTTON_LONG_PRESS:
        esp_restart();

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Button monitor tasks

      case EvCode::BUTTON_DOWN:
        button_down();
        break;

      case EvCode::BUTTON_UP:
        button_up();
        break;

      default:
        log("AP loop: Ignoring event %d\n", (int)ev.code);
        // Ignore the event
        break;
    }
  }
}

#endif
