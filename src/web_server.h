// Support for the device acting as a simple web server, for querying data across HTTP

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"
#include "sensor.h"

#ifdef WEB_SERVER
void start_web_server();
void maybe_handle_web_request(const SnappySenseData& data);
static const unsigned long WEB_SERVER_WAIT_TIME_MS = 100;
#endif

#endif // web_server_h_included
