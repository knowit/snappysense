// Support for the device acting as a simple web server

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"

#ifdef SNAPPY_WEB_SERVER

// Create a server for HTTP connections on the port, and start listening, but don't start polling
// for traffic yet.  There can be only one web server.
void web_server_init(int port);

// Start polling for traffic.
void web_server_start();

// Stop polling for traffic.
void web_server_stop();

// The request `r` was sent from the server to the main thread for processing.  This is a callback
// from the main thread that `r` has been processed and can be deleted.
void web_server_request_completed(WebRequest* r);

// Perform web polling work.
void web_server_poll();

#endif // SNAPPY_WEB_SERVER

#endif // web_server_h_included
