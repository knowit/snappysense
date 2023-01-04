// Code for uploading data to a remote HTTP server.
//
// The remote server must know how to handle POST to /data with a JSON object payload.
// For a simple server that can do this, see `../server`.

#ifndef web_upload_h_included
#define web_upload_h_included

#include "main.h"
#include "sensor.h"

#ifdef WEB_UPLOAD
void upload_results_to_http_server(const SnappySenseData& data);
#endif

#endif // !web_upload_h_included
