// Stuff to obtain the current time from a web server and to obtain timestamps with that adjustment.
//
// TODO: This is hacky, it can be integrated with Posix time code by using settimeofday() after
// obtaining the time base.

#include "snappytime.h"
#include "config.h"
#include "network.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#ifdef TIMESTAMP
// The time at startup.  We get this from a server in configureTime(), and use it to
// adjust the time read from the device when we report new readings.
static unsigned long timebase;
static bool time_configured;

void configure_time() {
  if (time_configured) {
    return;
  }
  time_configured = true;
  auto holder = connect_to_wifi();
  WiFiClient wifiClient;
  HTTPClient httpClient;
  // GET /time returns a number, representing the number of seconds UTC since the start
  // of the Posix epoch.
  httpClient.begin(wifiClient, time_server_host(), time_server_port(), "/time");
  httpClient.GET();
  // TODO: Probably want some integrity checking here; for example, the format of the
  // returned value could be "<...>" so that we could check that we've gotten a complete
  // value, and we should check the return value from sscanf.
  sscanf(httpClient.getString().c_str(), "%lu", &timebase);
  httpClient.end();
  wifiClient.stop();
}

time_t get_time() {
  if (!time_configured) {
    configure_time();
  }
  return time(nullptr) + timebase;
}

struct tm snappy_local_time() {
  time_t the_time = get_time();
  // FIXME: localtime wants a time zone to be set...
  struct tm the_local_time;
  localtime_r(&the_time, &the_local_time);
  return the_local_time;
}
#endif // TIMESTAMP
