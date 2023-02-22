// Support for the device acting as a simple web server, for configuration and commands

// TODO: Rename this as web_config.cpp

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"

#ifdef WEB_CONFIGURATION

bool web_start(int port);
void web_request_handled(WebRequest* r);
void web_poll();

#endif // WEB_CONFIGURATION

#endif // web_server_h_included
