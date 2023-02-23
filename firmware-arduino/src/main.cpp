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
// Follow breadcrumbs from the definition of TIMESERVER in main.h for more information.
// A simple time server is in the test-server/ directory in the present repo.
//
// The device can be configured to upload results to an MQTT broker (typically AWS)
// or to a Web server (for development and testing), or to neither.  For uploading to
// work, networking configuration variables have to be set, see CONFIG.md.  Also,
// appropriate servers will have to be running.  There is a simple server for http
// upload in the test-server/ directory in the present repo.
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


#include "config.h"
#include "config_ui.h"
#include "command.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "mqtt_upload.h"
#include "network.h"
#include "piezo.h"
#include "sensor.h"
#include "snappytime.h"
#include "slideshow.h"
#include "web_server.h"
#include "web_upload.h"

static void button_down();
static void button_up();
static void button_init();

// Whether the slideshow mode is enabled or not.  The default is to start the slideshow on startup.
// It can be toggled to monitoring mode by a press on the wake button.  Slideshow mode really only
// affects what we do after communication and before monitoring, and for how long.
#ifdef SLIDESHOW_MODE
bool slideshow_mode = true;
#else
bool slideshow_mode = false;
#endif // SLIDESHOW_MODE

// The timer driving the main loop
static TimerHandle_t main_task_timer;

// The timer driving the slideshow / display task
static TimerHandle_t slideshow_timer;

#ifdef SNAPPY_SERIAL_INPUT
// The timer driving the serial line poller, when serial line input is enabled
static TimerHandle_t serial_timer;
#endif

#define SHORT_PRESS_MAX 1999
#define LONG_PRESS_MIN 3000

// The event queue that drives all activity except within the music player task
static QueueHandle_t/*<int>*/ main_event_queue;

void setup() {
  main_event_queue = xQueueCreate(100, sizeof(SnappyEvent));

  // Power up the device.
  device_setup();
  // Serial port and display are up now and can be used for output.

  // Load config from nonvolatile memory, if available, otherwise use default values.
  read_configuration();

  // We sometimes need random numbers, try to seed the stream.
  randomSeed(entropy());

  // TODO: Not totally obvious that these should be here and not in loop(), following
  // the setting up of the start messages.  In principle some of these init functions
  // can themselves send messages to the main queue, or set up timers that cause
  // such messages to be sent, and we don't want that.
#ifdef SNAPPY_WIFI
  wifi_init();
#endif
#ifdef TIMESERVER
  timeserver_init();
#endif
#ifdef MQTT_UPLOAD
  mqtt_init();
#endif
  monitoring_init();
  button_init();

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

static void main_timer_tick(TimerHandle_t t) {
  put_main_event(EvCode((uint32_t)pvTimerGetTimerID(t)));
}

static void reset_main_timer(unsigned timeout_ms, EvCode payload) {
  vTimerSetTimerID(main_task_timer, (void*)payload);
  xTimerChangePeriod(main_task_timer, pdMS_TO_TICKS(timeout_ms), portMAX_DELAY);
}

static void slideshow_timer_tick(TimerHandle_t t) {
  put_main_event(EvCode::SLIDESHOW_TICK);
}

static void start_slideshow_task() {
  slideshow_timer_tick(slideshow_timer);
}

static void stop_slideshow_task() {
  xTimerStop(slideshow_timer, portMAX_DELAY);
}

static void advance_slideshow_task() {
  xTimerChangePeriod(slideshow_timer, pdMS_TO_TICKS(slideshow_update_interval_s() * 1000), portMAX_DELAY);
}

#ifdef WEB_CONFIGURATION
static void ap_mode_loop() NO_RETURN;
#endif

// This never returns.  The Arduino main loop does nothing interesting for us.  Our main loop
// is based on FreeRTOS and is not busy-waiting.

void loop() {
  /////////////////////////////////////////////////////////////////////////////////////////
  //
  // Main task's state

#ifdef SNAPPY_WIFI
  // True iff the main loop is in the communication window
  bool in_communication_window = false;

  bool in_wifi_window = false;
#endif

  // True iff the main loop is in the monitoring window (see below)
  bool in_monitoring_window = false;

  // True when the peripherals have been powered up
  bool is_powered_up = true;

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

  // True iff the device is in slideshow mode and the slideshow is running, this flag
  // is used to discard spurious slideshow ticks when the show is not supposed to
  // be running
  bool is_slideshow_running = false;

  // Create all the timers managed by the main loop.  TODO: Error handling?
  main_task_timer = xTimerCreate("main", 1, pdFALSE, nullptr, main_timer_tick);
  slideshow_timer = xTimerCreate("slideshow", 1, pdFALSE, nullptr, slideshow_timer_tick);
#ifdef SNAPPY_SERIAL_INPUT
  serial_timer = xTimerCreate("serial", 100, pdFALSE, nullptr,
                              [](TimerHandle_t){ put_main_event(EvCode::SERIAL_POLL); });
  xTimerStart(serial_timer, portMAX_DELAY);
#endif

  // The slideshow task starts whether we're in slideshow mode or not, since slideshow
  // mode only affects what happens between communication and monitoring, and for how long.
  // The first thing the task does is show the splash screen.
  put_main_event(EvCode::SLIDESHOW_START);

  // Start the main task.
  put_main_event(EvCode::START_CYCLE);

  // Shut up the compiler
  (void)in_monitoring_window;

  // This is used to improve the UX.  It shortens the comm window the first time around and
  // skips the relaxation / sleep before we read the sensors.
  bool first_time = true;

  // TODO: With a web server command task, the wifi is on all the time.  But this is basically
  // nuts, it warms up the device and has very limited utility since there are almost no commands
  // left, they are all development-oriented, and the developer has a serial line or terminal.
  // So get rid of the web command task.

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
#ifdef TIMESERVER
        comm_work = comm_work || timeserver_have_work();
#endif
#ifdef MQTT_UPLOAD
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
        //
        // TODO: This can take multiple retries and we don't want to block, so how do we deal with that?
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
        // TODO: Start communicating
        in_communication_window = true;
#ifdef TIMESERVER
        timeserver_start();
#endif
#ifdef MQTT_UPLOAD
        mqtt_start();
#endif
        unsigned long timeout_ms = comm_activity_timeout_s() * 1000;
        if (first_time) {
          timeout_ms /= 2;
        }
        reset_main_timer(timeout_ms, EvCode::COMM_ACTIVITY_EXPIRED);
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
          reset_main_timer(timeout_ms, EvCode::COMM_ACTIVITY_EXPIRED);
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
#ifdef MQTT_UPLOAD
          mqtt_stop();
#endif
#ifdef TIMESERVER
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
          reset_main_timer(comm_relaxation_timeout_s() * 1000, EvCode::SLEEP_START);
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
          if (!slideshow_mode) {
            put_main_event(EvCode::SLIDESHOW_STOP);
            reset_main_timer(monitoring_mode_sleep_s() * 1000, EvCode::POST_SLEEP);
            log("Nap time.  Sleep mode activated.\n");
            power_peripherals_off();
            is_powered_up = false;
          } else {
            reset_main_timer(slideshow_mode_sleep_s() * 1000, EvCode::POST_SLEEP);
          }
        }
        break;

      case EvCode::POST_SLEEP:
        if (!is_powered_up) {
          power_peripherals_on();
          log("Is anyone there?\n");
          put_main_event(EvCode::SLIDESHOW_RESET);
          put_main_event(EvCode::SLIDESHOW_START);
        }
        put_main_event(EvCode::MONITOR_START);
        first_time = false;
        break;

      case EvCode::MONITOR_START:
        log("Monitoring window opens\n");
        // Eventually the monitoring code will send MONITOR_STOP.
        monitoring_start();
        in_monitoring_window = true;
        break;

      case EvCode::MONITOR_STOP:
        log("Monitoring window closes\n");
        monitoring_stop();
        in_monitoring_window = false;
        put_main_event(EvCode::START_CYCLE);
        break;

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Events delivered to the main task

      case EvCode::MONITOR_DATA: {
        log("Monitor data received\n");
        // monitor data arrived after closing the monitoring window
        SnappySenseData* new_data = (SnappySenseData*)ev.pointer_data;
        assert(new_data != nullptr);
#ifdef MQTT_UPLOAD
        // TODO: This should only happen "every so often", possibly not every time there's new
        // data!  MQTT capture should have its own cadence, different from the needs of eg
        // the slideshow.  In some sense, MONITOR_DATA should cache the data object in this
        // function's local state and then there should be other timers driving the needs of
        // the slideshow and the MQTT system.  But this logic can be hidden inside mqtt_add_data,
        // too.
        mqtt_add_data(new SnappySenseData(*new_data));   // Make a copy
#endif
#ifdef SNAPPY_COMMAND_PROCESSOR
        command_data = *new_data;
#endif
        // slideshow_new_data takes over the ownership of the new_data.  See TODO above.
        slideshow_new_data(new_data);
        break;
      }

      case EvCode::BUTTON_PRESS:
        if (!is_powered_up) {
          // TODO: Observe that the button wakes us up if we're asleep but does not trigger
          // any change to the state machine.  If the monitoring window is scheduled to
          // run 30min from now, that does not change.  That's fine if we're in monitoring
          // mode and we stay there, but if we advance to slideshow mode it's not what we
          // want: we want the device to become active.  So in that case, cancel the timer
          // and move us along, if appropriate.  Can be a little tricky.  We might need a
          // bool to wrap the window when we're sleeping.
          power_peripherals_on();
          is_powered_up = true;
          log("Is anyone there?\n");
        } else {
          slideshow_next_mode = !slideshow_next_mode;
        }
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

      case EvCode::ACTUATOR:
        log("Ignoring actuator event, IMPLEMENTME\n");
        delete (Actuator*)ev.pointer_data;
        break;

#ifdef SNAPPY_COMMAND_PROCESSOR
      case EvCode::PERFORM: {
        String* cmd = (String*)ev.pointer_data;
        command_evaluate(*cmd, command_data, Serial);
        delete cmd;
        break;
      }
#endif

#ifdef WEB_CONFIGURATION
      case EvCode::BUTTON_LONG_PRESS:
        // Major mode change.  This is special: it knows a bit too much about the rest of the
        // state machine but that's just how it's going to be.  We're trying to avoid having
        // to process any other messages before switching to a completely different mode.
        // We never switch back: we restart the device when AP mode ends.
        if (!is_powered_up) {
          // We need the screen.
          power_peripherals_on();
          is_powered_up = true;
          log("Powered up for AP mode\n");
        }
        if (in_monitoring_window) {
          // This must stop all timers in the monitoring code
          monitoring_stop();
          in_monitoring_window = false;
        }
        if (in_communication_window) {
          // This must stop all timers in the communication code
#ifdef MQTT_UPLOAD
          mqtt_stop();
#endif
#ifdef TIMESERVER
          timeserver_stop();
#endif
          in_communication_window = false;
        }
        xTimerStop(main_task_timer, portMAX_DELAY);
        xTimerStop(slideshow_timer, portMAX_DELAY);
#ifdef SNAPPY_SERIAL_INPUT
        xTimerStop(serial_timer, portMAX_DELAY);
#endif
        // This never returns, it restarts the device.
        ap_mode_loop();
#endif

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Monitor task

      case EvCode::MONITOR_TICK:
        // Internal clock tick used for the monitor, with some unknown payload
        monitoring_tick(ev.scalar_data);
        break;

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Communication task

#ifdef MQTT_UPLOAD
      case EvCode::COMM_MQTT_WORK:
        mqtt_work();
        break;
#endif
#ifdef TIMESERVER
      case EvCode::COMM_TIMESERVER_WORK:
        timeserver_work();
        break;
#endif

      /////////////////////////////////////////////////////////////////////////////////////
      //
      // Display and slideshow task

      case EvCode::MESSAGE:
        slideshow_show_message_once((String*)ev.pointer_data);
        if (is_slideshow_running) {
          advance_slideshow_task();
          put_main_event(EvCode::SLIDESHOW_TICK);
        }
        break;

      case EvCode::SLIDESHOW_RESET:
        slideshow_reset();
        break;

      case EvCode::SLIDESHOW_START:
        if (!is_slideshow_running) {
          start_slideshow_task();
          is_slideshow_running = true;
        }
        break;

      case EvCode::SLIDESHOW_STOP:
        if (is_slideshow_running) {
          stop_slideshow_task();
          is_slideshow_running = false;
        }
        break;

      case EvCode::SLIDESHOW_TICK:
        if (is_slideshow_running) {
          slideshow_next();
          advance_slideshow_task();
        }
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
      case EvCode::SERIAL_POLL:
        serial_poll();
        xTimerReset(serial_timer, portMAX_DELAY);
        break;
#endif

      default:
        // WEB_POLL, WEB_REQUEST, WEB_REQUEST_FAILED are used only in AP mode
        panic("Unknown event");
    }
  }
}

#ifdef WEB_CONFIGURATION

// Event processing loop that never returns.  Processes events from the main queue, but
// ignores most of them.  In particular, the serial line commands are not available in
// this mode.

static void ap_mode_loop() {
  if (!start_access_point()) {
    panic("Access point failed");
  }
  if (!web_start(80)) {
    panic("Web config failed");
  }
  TimerHandle_t web_timer = xTimerCreate("web", pdMS_TO_TICKS(1000), pdTRUE, nullptr,
                                         [](TimerHandle_t){ put_main_event(EvCode::WEB_POLL); });
  xTimerStart(web_timer, portMAX_DELAY);

  for (;;) {
    SnappyEvent ev;
    while (xQueueReceive(main_event_queue, &ev, portMAX_DELAY) != pdTRUE) {}
    //log("AP event %d\n", (int)ev.code);
    switch (ev.code) {

      case EvCode::WEB_POLL:
        web_poll();
        break;

      case EvCode::WEB_REQUEST: {
        WebRequest* r = (WebRequest*)ev.pointer_data;
        process_config_request(r->client, r->request);
        web_request_handled(r);
        break;
      }

      case EvCode::WEB_REQUEST_FAILED: {
        WebRequest* r = (WebRequest*)ev.pointer_data;
        failed_config_request(r->client, r->request);
        web_request_handled(r);
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

/////////////////////////////////////////////////////////////////////////////////////////
//
// Button listener task's state

static bool button_is_down;
static struct timeval button_down_time;
static TimerHandle_t button_timer;

static void button_wdt(TimerHandle_t t) {
  if (!button_is_down) {
    return;
  }
  button_is_down = false;
  put_main_event(EvCode::BUTTON_LONG_PRESS);
}

static void button_init() {
  button_timer = xTimerCreate("button", LONG_PRESS_MIN, pdFALSE, nullptr, button_wdt);
}

static void button_down() {
  if (button_timer == nullptr) {
    return;
  }
  // ISR is signaling that the button is down
  // tv is the time we pressed it
  button_is_down = true;
  gettimeofday(&button_down_time, nullptr);
  xTimerReset(button_timer, portMAX_DELAY);
}

static void button_up() {
  if (!button_is_down) {
    return;
  }
  xTimerStop(button_timer, portMAX_DELAY);
  button_is_down = false;
  struct timeval now;
  gettimeofday(&now, nullptr);
  uint64_t press_ms = ((uint64_t(now.tv_sec) * 1000000 + now.tv_usec) -
                       (uint64_t(button_down_time.tv_sec) * 1000000 + button_down_time.tv_usec)) / 1000;
  if (press_ms > 100 && press_ms <= SHORT_PRESS_MAX) {
    put_main_event(EvCode::BUTTON_PRESS);
  } else if (press_ms >= LONG_PRESS_MIN) {
    put_main_event(EvCode::BUTTON_LONG_PRESS);
  }
}
