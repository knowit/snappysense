// Support for the device acting as a simple web server

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"

#ifdef WEB_CONFIGURATION

// Start listening for HTTP connections on the port.  There can be only one active web server,
// and once set up it actively polls on the incoming connection.
bool web_start(int port);

// The request `r` was sent from the server to the main thread for processing.  This is a callback
// from the main thread that `r` has been processed and can be deleted.
void web_request_completed(WebRequest* r);

// Perform web polling work.
void web_poll();

#endif // WEB_CONFIGURATION

#endif // web_server_h_included
