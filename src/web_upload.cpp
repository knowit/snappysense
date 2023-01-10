// Code for uploading JSON data to a remote HTTP server.

#include "web_upload.h"

#ifdef WEB_UPLOAD

#include "config.h"
#include "log.h"
#include "network.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

void WebUploadTask::execute(SnappySenseData* data) {
  if (data->sequence_number == 0) {
    // Mostly bogus data
    return;
  }

  log("Web: uploading data\n");

  String payload = format_readings_as_json(*data);

  // FIXME: There are lots of failure conditions here, and they all need to be checked somehow.
  //
  // FIXME: Basically the HTTPClient framework is not reliable.  Looking at addHeader(), for
  // example, it can OOM silently because it uses String without checking for OOM at all.
  // For another example, sendRequest() takes a String *by copy* for the payload; the copy can
  // fail due to OOM (in addition to being inefficient), but sendRequest() does not check
  // whether the string it receives is valid.
  //
  // In practice this is not going to be good enough.  It is probably indicative of the quality
  // of the Arduino libraries - "maker" quality, not "production".
  
  auto holder(connect_to_wifi());
  WiFiClient wifiClient;
  HTTPClient httpClient;
  httpClient.begin(wifiClient, web_upload_host(), web_upload_port(), "/data");
  httpClient.addHeader(String("Content-Type"), String("application/json"));
  httpClient.sendRequest("POST", payload);
  httpClient.end();
  wifiClient.stop();
}

#endif // WEB_UPLOAD
