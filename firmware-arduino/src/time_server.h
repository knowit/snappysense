// Stuff to obtain the current time from a web server and configure the clock.

#ifndef time_server_h_included
#define time_server_h_included

#include "main.h"

#ifdef SNAPPY_NTP

// Call this before anything else.
void timeserver_init();

// Return true if the time server needs WiFi to be enabled because it has work to do.
bool timeserver_have_work();

// WIFI must be up.  Try to connect to the time server, if the time has not been set
// already.  This will result in COMM_NTP_WORK messages being posted on the
// main queue every so often if retries are required.
void timeserver_start();

// Called from the main loop in response to COMM_NTP_WORK messages.
void timeserver_work();

// Stop trying to connect to the time server, if that's still going on.  Can be called
// without start having been called first.
void timeserver_stop();

// This is either 0 (time has not been configured) or a number of seconds to be added
// to time readings that were made before time was adjusted in order to bring them
// into the present.
time_t time_adjustment();

#endif

#endif // !time_server_h_included
