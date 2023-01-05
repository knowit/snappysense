// Support for the device acting as a simple web server, for querying data across HTTP

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"
#include "sensor.h"

#ifdef WEB_SERVER
// This brings up WiFi and starts listening.  It will keep the device alive.
void start_web_server();

// Accept a connect request if there is one and read from it.  THIS WILL BLOCK UNTIL
// THE ENTIRE REQUEST HAS BEEN READ!  Once the complete request has been read,
// it is processed by the command processor, which will produce some output that
// is sent back to the client.  The connection is then closed.
void maybe_handle_web_request(const SnappySenseData& data);
#endif

#endif // web_server_h_included
