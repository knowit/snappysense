// Snappysense program configuration and base include file.

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

// In this mode, the display is updated frequently with readings.  When
// not in this mode, the display sleeps, and the device wakes up to perform readings,
// report them, and (if configured), process interactive commands.
#define STANDALONE

// Stamp uploaded records with the current time.  For this to work, the time has to
// be configured at startup, incurring a little extra network traffic.  See snappytime.h.
#define TIMESTAMP

// With WEB_UPLOAD, the device will upload readings to a predefined http server
// every so often.  See web_upload.h.
//#define WEB_UPLOAD

// With MQTT_UPLOAD, the device will upload readings to a predefined mqtt broker
// every so often.  See mqtt_upload.h.
#define MQTT_UPLOAD

// With WEB_SERVER, the device creates a server on port 8088 and listens for 
// commands, just ask for / or /help to see a directory of the possible requests.
//#define WEB_SERVER

// With SERIAL_SERVER, the device listens for commands on the serial line, the
// command "help" will provide a list of possible commands.
#define SERIAL_SERVER

// Include the log(stream, fmt, ...) functions, see log.h
#define LOGGING

// On Hardware 1.0.0 this requires all wifi functionality to be disabled
//#define TEST_MEMS

// END FUNCTIONAL CONFIGURATION
//
////////////////////////////////////////////////////////////////////////////////

#if defined(WEB_UPLOAD) || defined(WEB_SERVER) || defined(MQTT_UPLOAD) || defined(TIMESTAMP)
# define HAVE_WIFI
#endif

#ifdef HARDWARE_1_0_0
// In V1.0.0, there's a resource conflict between WiFi and the noise sensor / mic.
# if !defined(HAVE_WIFI)
#  define READ_NOISE
# endif
#endif // HARDWARE_1_0_0

#endif // !main_h_included
