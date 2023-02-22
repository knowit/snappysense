#ifndef config_ui_h_included
#define config_ui_h_included

#include "main.h"

#ifdef WEB_CONFIGURATION

bool start_access_point();
void process_config_request(Stream& client, const String& request);
void failed_config_request(Stream& client, const String& request);

#endif // WEB_CONFIGURATION

#endif // !config_ui_h_included
