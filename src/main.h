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
//   enough to handle OOM reliably anyway.  Basically, just ignore the problem.
// - Avoid using libraries that depend on exception handling, or handle exceptions
//   close to API calls if necessary.
// - Naming conventions: functions and variables and file names are snake_case,
//   types are CamelCase, global constants are ALL_UPPER_SNAKE_CASE.
// - Indent using spaces, not tabs.  Preferably 2 spaces per level.

////////////////////////////////////////////////////////////////////////////////
//
// FUNCTIONAL CONFIGURATION

// Hardware version you're compiling for.  Pick one.  See device.cpp for more.
#define HARDWARE_1_0_0
//#define HARDWARE_1_1_0

// Set this to make config.cpp include development_config.h with compiled-in values
// and short time intervals for many things (to speed up testing).
// Otherwise, we're in "production" mode and default values are mostly blank and
// the device must be provisioned from interactive config mode.
#define DEVELOPMENT

// Stamp uploaded records with the current time.  For this to work, the time has to
// be configured at startup, incurring a little extra network traffic, and a time server
// has to be provisioned.  See snappytime.h.
#define TIMESTAMP

// With MQTT_UPLOAD, the device will upload readings to a predefined mqtt broker
// every so often.  See mqtt_upload.h.
#define MQTT_UPLOAD

// Include the log(stream, fmt, ...) functions, see log.h.  If the serial device is
// connected then log messages will appear there, otherwise they will be discarded.
#define LOGGING

// To enter interactive configuration mode:
//  - connect the device to USB and open a serial console
//  - press and hold the wake pin and then press and release the reset button
//  - there will be a message INTERACTIVE CONFIGURATION MODE in the console
//    and if there's a display there will be a message CONFIGURATION
//    on it
//  - there will be a menu system for configuring the device
#define INTERACTIVE_CONFIGURATION

// WEB_CONFIGURATION causes a WiFi access point to be created with an SSID
// printed on the display when the device comes up in config mode (see
// INTERACTIVE_CONFIGURATION, which is required by WEB_CONFIGURATION).
// The user can connect to this network and request "/".  This will return
// an HTML page that contains a form that is submitted with "POST /" and
// can be used to set some simple parameters in the configuration.
// (Conflicts with WEB_COMMAND_SERVER.)
#define WEB_CONFIGURATION

// In this mode, the display is updated frequently with readings.  The device and
// display are on continually, and the device uses a lot of power.  It is useful in
// production only when we can count on a non-battery power source.
#define SLIDESHOW_MODE

// ----------------------------------------------------------------------------
// The following are mostly useful during development and would not normally be
// enabled in production.

// With SERIAL_SERVER, the device listens for commands on the serial line, the
// command "help" will provide a list of possible commands.
//#define SERIAL_SERVER

// With WEB_UPLOAD, the device will upload readings to a predefined http server
// every so often.  See web_upload.h.
//#define WEB_UPLOAD

// Simple web server to send commands to the device, obtain data, etc.  The IP address
// of the device is printed on the serial line and also on the display.  Just ask
// for / or /help to see a directory of the possible requests.
// (Conflicts with WEB_CONFIGURATION.)
//#define WEB_COMMAND_SERVER

// (Obscure) This tests the sensitivity and readings of the MEMS unit, if you know
// what you're looking at.
//
// Note, on Hardware 1.0.0 this requires all wifi functionality to be disabled.
//#define TEST_MEMS

// (Obscure) This sets the power-off interval artificially low so that it's
// easier to test it during development.
//#define TEST_POWER_MANAGEMENT

// END FUNCTIONAL CONFIGURATION
//
////////////////////////////////////////////////////////////////////////////////

#if defined(WEB_CONFIGURATION) || defined(WEB_COMMAND_SERVER)
# define WEB_SERVER
#endif

#if defined(WEB_UPLOAD) || defined(WEB_SERVER) || defined(MQTT_UPLOAD) || defined(TIMESTAMP) || defined(INTERACTIVE_CONFIGURATION)
# define SNAPPY_WIFI
#endif

#if defined(SERIAL_SERVER) || defined(WEB_SERVER)
# define SNAPPY_COMMAND_PROCESSOR
#endif

#if defined(SERIAL_SERVER) || defined(INTERACTIVE_CONFIGURATION)
# define SNAPPY_SERIAL_LINE
#endif

#if !defined(DEVELOPMENT)
# ifdef SLIDESHOW_MODE
#  warning "SLIDESHOW_MODE not usually enabled in production"
# endif
# ifdef SERIAL_SERVER
#  warning "SERIAL_SERVER not usually enabled in production"
# endif
# ifdef WEB_UPLOAD
#  warning "WEB_UPLOAD not usually enabled in production"
# endif
# ifdef WEB_COMMAND_SERVER
#  warning "WEB_COMMAND_SERVER not usually enabled in production"
# endif
# ifdef TEST_MEMS
#  error "TEST_MEMS is incompatible with production mode"
# endif
#endif

#if defined(TEST_POWER_MANAGEMENT) && (defined(SLIDESHOW_MODE) || \
                                       defined(DEVELOPMENT) || \
                                       defined(SNAPPY_SERIAL_LINE) || \
                                       defined(SNAPPY_WEB_SERVER))
# error "Power management test won't work when the device is mostly busy"
#endif

#if defined(WEB_CONFIGURATION) && !defined(INTERACTIVE_CONFIGURATION)
# error "WEB_CONFIGURATION requires INTERACTIVE_CONFIGURATION."
#endif

#ifdef HARDWARE_1_0_0
// In V1.0.0, there's a resource conflict between WiFi and the noise sensor / mic.
# if !defined(SNAPPY_WIFI)
#  define READ_NOISE
# endif
#endif // HARDWARE_1_0_0

#endif // !main_h_included
