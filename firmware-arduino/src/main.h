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

// Set this to make config.cpp include development_config.h with compiled-in values
// and short time intervals for many things (to speed up testing).
// Otherwise, we're in "production" mode and default values are mostly blank and
// the device must be provisioned from interactive config mode.
//#define SNAPPY_DEVELOPMENT

// With SNAPPY_NTP, synchronize the time with an ntp server at startup (only).
// See time_server.h.
#define SNAPPY_NTP

// With SNAPPY_TIMESTAMPS, every sent message gets a timestamp, and no messages
// are sent until the time has been configured.  Requires SNAPPY_NTP or SNAPPY_LORA
// for the time service.
#define SNAPPY_TIMESTAMPS

// With SNAPPY_MQTT, the device will upload readings to a predefined mqtt broker
// every so often, and download config messages and commands.  See mqtt.h.
#define SNAPPY_MQTT

// SNAPPY_WEBCONFIG causes a WiFi access point to be created with an SSID
// printed on the display when the device comes up in config mode, allowing for both
// factory provisioning of ID, certificates, and so on, as well as user provisioning
// of network names and other local information.  See CONFIG.md at the root of the
// repo.
#define SNAPPY_WEBCONFIG

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

// END FUNCTIONAL CONFIGURATION
//
////////////////////////////////////////////////////////////////////////////////

#if defined(SNAPPY_TIMESTAMPS) && !defined(SNAPPY_NTP)
# error "SNAPPY_TIMESTAMPS requires a time server"
#endif

#if defined(SNAPPY_WEBCONFIG) || defined(SNAPPY_MQTT) || defined(SNAPPY_NTP)
# define SNAPPY_WIFI
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

#endif // !main_h_included
