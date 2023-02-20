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
//#define HARDWARE_1_0_0
#define HARDWARE_1_1_0

// Set this to make config.cpp include development_config.h with compiled-in values
// and short time intervals for many things (to speed up testing).
// Otherwise, we're in "production" mode and default values are mostly blank and
// the device must be provisioned from interactive config mode.
//#define DEVELOPMENT

// Stamp uploaded records with the current time.  For this to work, the time has to
// be configured at startup, incurring a little extra network traffic, and a time server
// has to be provisioned.  See snappytime.h.
//#define TIMESERVER

// With MQTT_UPLOAD, the device will upload readings to a predefined mqtt broker
// every so often.  See mqtt_upload.h.
//#define MQTT_UPLOAD

// Accept 'snappy/command/<device-name> messages from the server.  Currently none are
// defined, so we don't normally accept them.
//#define MQTT_COMMAND_MESSAGES

// Include the log(stream, fmt, ...) functions, see log.h.  If the serial device is
// connected then log messages will appear there, otherwise they will be discarded.
#define LOGGING

// WEB_CONFIGURATION causes a WiFi access point to be created with an SSID
// printed on the display when the device comes up in config mode, allowing for both
// factory provisioning of ID, certificates, and so on, as well as user provisioning
// of network names and other local information.  See CONFIG.md at the root of the
// repo.
//#define WEB_CONFIGURATION

// In this mode, the display is updated frequently with readings.  The device and
// display are on continually, and the device uses a lot of power.  It is useful in
// production only when we can count on a non-battery power source.
#define SLIDESHOW_MODE

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

// Play a tune when starting the device
//#define STARTUP_SONG

// With SERIAL_COMMAND_SERVER, the device listens for commands on the serial line, the
// command "help" will provide a list of possible commands.
//#define SERIAL_COMMAND_SERVER

// With WEB_UPLOAD, the device will upload readings to a predefined http server
// every so often.  See web_upload.h.
//#define WEB_UPLOAD

// Simple web server to send commands to the device, obtain data, etc.  The IP address
// of the device is printed on the serial line and also on the display.  Just ask
// for / or /help to see a directory of the possible requests.
//#define WEB_COMMAND_SERVER

// (Obscure) This sets the power-off interval artificially low so that it's
// easier to test it during development.
//#define TEST_POWER_MANAGEMENT

// END FUNCTIONAL CONFIGURATION
//
////////////////////////////////////////////////////////////////////////////////

#if defined(WEB_CONFIGURATION) || defined(WEB_COMMAND_SERVER)
# define WEB_SERVER
#endif

#if defined(WEB_UPLOAD) || defined(WEB_SERVER) || defined(MQTT_UPLOAD) || defined(TIMESTAMP) || defined(WEB_CONFIGURATION)
# define SNAPPY_WIFI
#endif

#if defined(SERIAL_COMMAND_SERVER) || defined(WEB_COMMAND_SERVER)
# define SNAPPY_COMMAND_PROCESSOR
#endif

#if defined(SERIAL_COMMAND_SERVER) || defined(SERIAL_CONFIGURATION)
# define SNAPPY_SERIAL_INPUT
#endif

#if !defined(HARDWARE_1_0_0)
//# define SNAPPY_PIEZO
#endif

#if !defined(DEVELOPMENT)
# ifdef SERIAL_COMMAND_SERVER
#  warning "SERIAL_COMMAND_SERVER not usually enabled in production"
# endif
# ifdef WEB_UPLOAD
#  warning "WEB_UPLOAD not usually enabled in production"
# endif
# ifdef WEB_COMMAND_SERVER
#  warning "WEB_COMMAND_SERVER not usually enabled in production"
# endif
#endif

#if defined(TEST_POWER_MANAGEMENT) && (defined(DEVELOPMENT) || \
                                       defined(SNAPPY_SERIAL_LINE) || \
                                       defined(SNAPPY_WEB_SERVER))
# error "Power management test won't work when the device is mostly busy"
#endif

// In V1.0.0, there's a resource conflict between WiFi and the noise sensor / mic.
#if defined(HARDWARE_1_0_0) && defined(SNAPPY_WIFI)
# undef SENSE_NOISE
#endif

// Altitude is broken on HW 1.x.y: it's a crude formula wrapped around the pressure sensor,
// this simply can't be right.
#if defined(HARDWARE_1_0_0) || defined(HARDWARE_1_1_0)
# undef SENSE_ALTITUDE
#endif

extern bool slideshow_mode;

// Event codes for events posted to main_event_queue

enum class EvCode {
  NONE = 0,

  // Main task
  START_CYCLE,
  COMM_START,
  COMM_FAILED,
  COMM_ACTIVITY,
  COMM_ACTIVITY_EXPIRED,
  POST_COMM,
  SLEEP_START,
  POST_SLEEP,
  MONITOR_START,
  MONITOR_STOP,
  MONITOR_DATA,
  MONITOR_TICK,
  BUTTON_PRESS,
  BUTTON_LONG_PRESS,
  AP_MODE,

  // Communication work
  COMM_POLL,

  // Slideshow/display work
  MESSAGE,
  SLIDESHOW_START,
  SLIDESHOW_STOP,
  SLIDESHOW_TICK,

  // Button listener
  BUTTON_DOWN,
  BUTTON_UP,

  // Serial listener
  SERIAL_POLL,
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
void put_main_event(EvCode code, int payload);

#endif // !main_h_included
