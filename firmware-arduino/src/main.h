// Snappysense program configuration and base include file.

// For a general overview, see comment block in main.cpp.

#ifndef main_h_included
#define main_h_included

#include "Arduino.h"

////////////////////////////////////////////////////////////////////////////////
//
// CODING STANDARDS
//
// - All <filename>.cpp files shall have a corresponding <filename>.h and the
//   <filename>.cpp file shall include <filename>.h first.
// - All <filename>.h shall include main.h first.
// - Do not worry about out-of-memory conditions (OOM) except perhaps for large
//   allocations (several KB at least).  The Arduino libraries are not reliable
//   enough to handle OOM properly anyway.  Basically, just ignore the problem.
// - Avoid using libraries that depend on exception handling, or handle exceptions
//   close to API calls if necessary.
// - Naming conventions: functions and variables and file names are snake_case,
//   types are CamelCase, global constants are ALL_UPPER_SNAKE_CASE.
// - Indent using spaces, not tabs.  Preferably 2 spaces per level.
// - Try to conform to whatever standard is used in the file you're editing, if
//   it differs (in reasonable ways) from the above.
// - Try to limit line lengths to 100 chars (allowing two side-by-side editors
//   on laptops and three on most standalone displays)

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTIONAL CONFIGURATION

// Hardware version you're compiling for.  Pick one.  See device.cpp for more.
//#define SNAPPY_HARDWARE_1_0_0
#define SNAPPY_HARDWARE_1_1_0

// Radio configuration.  Pick one (or none).
//
// LoRaWAN support is NOT IMPLEMENTED.
#define SNAPPY_WIFI
//#define SNAPPY_LORA

// Set this to make config.cpp include development_config.h with compiled-in values
// and short time intervals for many things (to speed up testing).
// Otherwise, we're in "production" mode and default values are mostly blank and
// the device must be provisioned from interactive config mode.
//#define SNAPPY_DEVELOPMENT

// With SNAPPY_TIMESTAMPS, every sent message gets a timestamp, and no messages
// are sent until the time has been configured.  Requires SNAPPY_NTP or SNAPPY_LORA
// for the time service.
#define SNAPPY_TIMESTAMPS

// With SNAPPY_DEEP_SLEEP, the system enters deep sleep mode in the sleep window.  This essentially
// means that it reboots completely when it comes out of sleep.  However, there are some
// differences from normal boot, at least these:
//  - no "startup" message should be sent, but info saved at sleep time should be used.
//    however, if no startup message has been sent since true boot, we should send it.
//    so there is a notion of whether "startup has succeeded"
//  - there will be no data left in the comm queue because there was no memory to save
//    it in, indeed the comm queue has very limited scope
//  - consequently, the device should start up in the POST_SLEEP state, not in the START_CYCLE
//    state, and should perform a measurement which it then displays and communicates
//  - if a datum can't be sent because there's no wifi it will be discarded, unless we can
//    save it to flash and send it later
//
// State saved in ULP
//  - whether a startup message has been sent / the startup process is complete
//  - if the startup process is complete, any settings received from the server during startup
//  - (whether there are messages on Flash to send, + info about them)
//  - probably the first_time flag
//  - the mode flag is implicitly "monitoring"
#define SNAPPY_DEEP_SLEEP

/////
//
// Profile 1: WiFi
//
// Here we have NTP, MQTT, and WiFi Access Point configuration.

// With SNAPPY_NTP, synchronize the time with an ntp server at startup (only).
// See time_server.h.  This requires SNAPPY_WIFI.
#define SNAPPY_NTP

// With SNAPPY_MQTT, the device will upload readings to a predefined mqtt broker
// every so often, and download config messages and commands.  Requires SNAPPY_WIFI.
#define SNAPPY_MQTT

// SNAPPY_WEBCONFIG causes a WiFi access point to be created with an SSID
// printed on the display when the device comes up in config mode, allowing for both
// factory provisioning of ID, certificates, and so on, as well as user provisioning
// of network names and other local information.  See CONFIG.md at the root of the
// repo.  Requires SNAPPY_WIFI.
#define SNAPPY_WEBCONFIG

/////
//
// Profile 2: LoRaWAN
//
// Here we have time service and message upload via LoRa, and factory config over I2C.
// There is no user config.
//
// LoRaWAN support is NOT IMPLEMENTED.

// SNAPPY_I2CCONFIG causes the device to go into I2C slave mode on address TBD when the
// device comes up in config mode, allowing for a connected I2C master to upload config
// data by writing to port TBD.  Requires !SNAPPY_WIFI because the mode that the device
// switches into on a long button press changes.
//
// SNAPPY_I2CCONFIG is NOT IMPLEMENTED.
//#define SNAPPY_I2CCONFIG

// Control the various sensors
#define SENSE_TEMPERATURE
#define SENSE_HUMIDITY
#define SENSE_UV
#define SENSE_LIGHT
#define SENSE_PRESSURE
#define SENSE_ALTITUDE
#define SENSE_AIR_QUALITY_INDEX
#define SENSE_TVOC
#define SENSE_CO2
#define SENSE_MOTION
#define SENSE_NOISE

// ----------------------------------------------------------------------------
// The following are mostly useful during development and would not normally be
// enabled in production.

// Include the log(stream, fmt, ...) functions, see log.h.  If the serial device is
// connected then log messages will appear there, otherwise they will be discarded.
#define LOGGING

// Play a tune when starting the device
//#define STARTUP_SONG

// With SERIAL_COMMAND_SERVER, the device listens for commands on the serial line, the
// command "help" will provide a list of possible commands.
//#define SERIAL_COMMAND_SERVER

// If enabled, trigger a long button press this many seconds after startup.  Useful for
// experimenting with the ESP32 config system off the PCB.
//#define SIMULATE_LONG_PRESS 3

// If enabled, trigger a short button press this many seconds after startup.  Useful for
// going into monitoring mode off the PCB.
//#define SIMULATE_SHORT_PRESS 120

// If enabled, ignore the button on the board.  Useful for experimenting with the ESP32
// and sensors off the board.
//#define DISABLE_BUTTON

#if (defined(SIMULATE_LONG_PRESS) || defined(SIMULATE_SHORT_PRESS)) && !defined(DISABLE_BUTTON)
# error "Unlikely configuration" // Uncomment this if it's in the way
#endif

// END FUNCTIONAL CONFIGURATION
//
////////////////////////////////////////////////////////////////////////////////

#if defined(SNAPPY_WIFI) && defined(SNAPPY_LORA)
# error "Only one radio type allowed"
#endif

#if defined(SNAPPY_TIMESTAMPS) && !(defined(SNAPPY_NTP) || defined(SNAPPY_LORA))
# error "SNAPPY_TIMESTAMPS requires a time server"
#endif

#if (defined(SNAPPY_WEBCONFIG) || defined(SNAPPY_MQTT) || defined(SNAPPY_NTP)) && !defined(SNAPPY_WIFI)
# error "SNAPPY_WIFI required for WEBCONFIG, MQTT, and NTP"
#endif

#if defined(SNAPPY_I2CCONFIG) && defined(SNAPPY_WIFI)
# error "SNAPPY_WEBCONFIG requires !SNAPPY_WIFI"
#endif

#if defined(SNAPPY_MQTT) || defined(SNAPPY_LORA)
# define SNAPPY_UPLOAD
#endif

#if defined(SERIAL_COMMAND_SERVER)
# define SNAPPY_COMMAND_PROCESSOR
#endif

#if defined(SERIAL_COMMAND_SERVER)
# define SNAPPY_SERIAL_INPUT
#endif

#if defined(SNAPPY_WEBCONFIG)
# define SNAPPY_WEB_SERVER
#endif

#if !defined(SNAPPY_HARDWARE_1_0_0)
# define SNAPPY_PIEZO
#endif

#if !defined(SNAPPY_DEVELOPMENT)
# ifdef SERIAL_COMMAND_SERVER
#  warning "SERIAL_COMMAND_SERVER not usually enabled in production"
# endif
#endif

// In V1.0.0, there's a resource conflict between WiFi and the noise sensor / mic.
#if defined(SNAPPY_HARDWARE_1_0_0) && defined(SNAPPY_WIFI)
# undef SENSE_NOISE
#endif

// Altitude is broken on HW 1.x.y: it's a crude formula wrapped around the pressure sensor,
// this simply can't be right.
#if defined(SNAPPY_HARDWARE_1_0_0) || defined(SNAPPY_HARDWARE_1_1_0)
# undef SENSE_ALTITUDE
#endif

#define WARN_UNUSED __attribute__((warn_unused_result))
#define NO_RETURN __attribute__ ((noreturn))

// Flag indicating whether the device is in slideshow mode or not.  This can be used esp
// by the prefs code to return different prefs values for slideshow mode than for monitoring
// mode.
extern bool slideshow_mode;

// Event codes for events posted to main_event_queue
enum class EvCode {
  NONE = 0,

  // Main task state machine (timer-driven and event-driven).  There's a
  // blurry line between these states and the external events, below,
  // but mainly, these states can cause state changes, while the events
  // below generally don't do that.
  START_CYCLE,
  COMM_START,
  COMM_WIFI_CLIENT_RETRY,
  COMM_WIFI_CLIENT_FAILED,
  COMM_WIFI_CLIENT_UP,
  COMM_ACTIVITY_EXPIRED,
  POST_COMM,
  SLEEP_START,
  POST_SLEEP,
  MONITOR_START,
  MONITOR_STOP,

  // External events being handled in the main task, orthogonally to its
  // state machine.
  COMM_ACTIVITY,      // Communication activity, from comm task
  MONITOR_DATA,       // Observation data, from monitor task; transfers a SnappySenseData object
  BUTTON_PRESS,       // Short press, from button listener
  BUTTON_LONG_PRESS,  // Long press, from button listener
  ENABLE_DEVICE,      // Enable monitoring, from comm task
  DISABLE_DEVICE,     // Disable monitoring, from comm task
  SET_INTERVAL,       // Set monitoring interval, from comm task
  PERFORM,            // Interactive command, from serial listener; transfers a String object
  WEB_REQUEST,        // Successful request, transfers a WebRequest object
  WEB_REQUEST_FAILED, // Failed request, transfers a WebRequest object

  // Monitoring task state machine (timer-driven)
  MONITOR_WORK,       // Payload: integer code

  // Communication task state machine (timer-driven)
  COMM_MQTT_WORK,
  COMM_NTP_WORK,

  // Slideshow/display task state machine (timer-driven)
  MESSAGE,
  SLIDESHOW_START,
  SLIDESHOW_RESET,
  SLIDESHOW_STOP,
  SLIDESHOW_WORK,

  // Button listener task state machine (interrupt-driven)
  BUTTON_DOWN,
  BUTTON_UP,

  // Serial listener task state machine (timer-driven)
  SERIAL_SERVER_POLL,

  // Web listener task state machine (timer-driven)
  WEB_SERVER_POLL,
};

struct SnappyEvent {
  SnappyEvent() : code(EvCode::NONE), pointer_data(nullptr) {}
  SnappyEvent(EvCode code) : code(code), pointer_data(nullptr) {}
  SnappyEvent(EvCode code, void* data) : code(code), pointer_data(data) { assert(data != nullptr); }
  SnappyEvent(EvCode code, uint32_t data) : code(code), scalar_data(data) {}
  EvCode code;
  union {
    void* pointer_data;
    uint32_t scalar_data;
  };
};

void put_main_event(EvCode code);
void put_main_event_from_isr(EvCode code);
void put_main_event(EvCode code, void* data);
void put_main_event(EvCode code, uint32_t payload);

struct WebRequest {
  WebRequest(String request, Stream& client) : request(request), client(client) {}
  String request;
  Stream& client;
};

// These are global state variables that need to be persisted during deep sleep.  They are grouped
// into substructures that are named after the modules that logically own them.  They are given initial
// values in main.cpp mostly by means of expressions defined in the appropriate header files.
//
// These are the canonical locations for these values - in particular, they are not copies of variables
// that exist elsewhere that have to be synced to the persistent storage.

struct PersistentData {
  struct {
    // The access point index that worked for us last time, it's where we start looking
    // next time we try to connect.
    int last_successful_access_point;
  } network_wifi;

  struct {
    // Set to true once the startup message has been sent and the startup process should be
    // considered complete.
    bool startup_message_sent;
  } mqtt;

  struct {
    // The "enabled" setting that may be received from the MQTT broker on startup but which
    // is not saved in the preferences.
    bool enabled;

    // The "interval" setting that may be received from the MQTT broker on startup but which
    // is not saved in the preferences.
    unsigned long monitoring_capture_interval_for_upload_s;
  } config;

  struct {
    // Set to true before going into deep sleep
    bool waking_from_deep_sleep;

    // This is used to improve the UX.  It shortens the comm window the first time around and
    // skips the relaxation / sleep before we read the sensors.  It is persisted because we really only
    // want to do this the first time the device boots up.
    bool first_time;
  } main;

  // TODO: What about early_times in the mqtt code?  (This is like first_time, it is logic that presupposes
  // some persistent state.)  This is a hack that makes us do mqtt work more often.  It is possible that the
  // right behavior here is that *while there is mqtt work to do* we should not enter deep sleep.  There is
  // mqtt work to do only if comms are possible, I think (certainly this ought to be the case).
};

extern PersistentData persistent_data;

#endif // !main_h_included
