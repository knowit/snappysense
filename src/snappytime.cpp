// Stuff to obtain the current time from a web server and to obtain timestamps with that adjustment.
//
// TODO: This is hacky, it can be integrated with Posix time code by using settimeofday() after
// obtaining the time base.

#include "snappytime.h"
#include "config.h"
#include "log.h"
#include "network.h"

#ifdef TIMESTAMP

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// The time at startup.  We get this from a server in configureTime(), and use it to
// adjust the time read from the device when we report new readings.
static unsigned long timebase;
static bool time_configured;

void configure_time() {
  if (time_configured) {
    return;
  }
  log("Time: obtaining current time from server\n");
  time_configured = true;
  auto holder = connect_to_wifi();
  WiFiClient wifiClient;
  HTTPClient httpClient;
  // GET /time returns a number, representing the number of seconds UTC since the start
  // of the Posix epoch.
  httpClient.begin(wifiClient, time_server_host(), time_server_port(), "/time");
  httpClient.GET();
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
  // FIXME: Issue 7: localtime wants a time zone to be set...
  struct tm the_local_time;
  localtime_r(&the_time, &the_local_time);
  return the_local_time;
}

String format_time(const struct tm& time) {
  static const char* const weekdays[] = {
    "sun", "mon", "tue", "wed", "thu", "fri", "sat"
  };
  char buf[256];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d/%s", 
           time.tm_year + 1900,     // year number
           time.tm_mon + 1,         // month, 1-12
           time.tm_mday,            // day of the month, 1-31
           time.tm_hour,            // hour, 0-23
           time.tm_min,             // minute, 0-59
           weekdays[time.tm_wday]); // day of the week
  return String(buf);
}
#endif // TIMESTAMP
