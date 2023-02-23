// Logic for setting up a configuration access point and handling web requests received
// by a server on that access point.

#ifndef web_config_h_included
#define web_config_h_included

#include "main.h"

#ifdef WEB_CONFIGURATION

bool webcfg_start_access_point();
void webcfg_process_request(Stream& client, const String& request);
void webcfg_failed_request(Stream& client, const String& request);

#endif // WEB_CONFIGURATION

#endif // !web_config_h_included
