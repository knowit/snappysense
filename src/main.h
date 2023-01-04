#ifndef main_h_included
#define main_h_included

#include "Arduino.h"

////////////////////////////////////////////////////////////////////////////////
//
// CODING STANDARDS FOR THE PROTOTYPE
//
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

// In this mode, the display is updated frequently with readings.  When
// not in this mode, the display mostly sleeps, waking up to perform readings
// and report them.
#define STANDALONE

// Stamp uploaded records with the current time.  For this to work, the time has to
// be configured at startup, incurring a little extra network traffic.  See snappytime.h.
#define TIMESTAMP

// With WEB_UPLOAD, the device will upload readings to a predefined http server
// every so often.  See web_upload.h.
#define WEB_UPLOAD

// With MQTT_UPLOAD, the device will upload readings to a predefined mqtt broker
// every so often.  See mqtt_upload.h.
#define MQTT_UPLOAD

// With WEBSERVER, the device creates a server on port 8088 and listens for 
// requests to report readings, just ask for / to see a directory of the
// possible requests.
//#define WEBSERVER

// Include the log(stream, fmt, ...) functions, see log.h
#define LOGGING

// END FUNCTIONAL CONFIGURATION
//
////////////////////////////////////////////////////////////////////////////////

// In V1.0.0, there's a hardware resource conflict between WiFi and the noise sensor.

#if !defined(WEB_UPLOAD) && !defined(WEBSERVER)
#  define READ_NOISE
#endif

// The code is not set up to be both a web server and a web client (yet)

#if defined(WEBSERVER) && defined(WEB_UPLOAD)
#  error "Config conflict"
#endif

#endif // !main_h_included
