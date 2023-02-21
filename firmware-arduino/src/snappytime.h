// Stuff to obtain the current time from a web server and configure the clock.

#ifndef snappytime_h_included
#define snappytime_h_included

#include "main.h"

#ifdef TIMESERVER

// Call this before anything else.
void timeserver_init();

// WIFI must be up.  Try to connect to the time server, if the time has not been set
// already.  This will result in COMM_TIMESERVER_WORK messages being posted on the
// main queue every so often if retries are required.
void timeserver_start();

// Called from the main loop in response to COMM_TIMESERVER_WORK messages.
void timeserver_work();

// Stop trying to connect to the time server, if that's still going on.
void timeserver_stop();

#endif

#endif // !snappytime_h_included
