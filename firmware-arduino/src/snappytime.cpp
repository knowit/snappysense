// Stuff to obtain the current time from a web server and configure the clock.
//
// The remote server must know how to handle GET to /time; it must respond with a payload that is
// the decimal encoding of the number of seconds elapsed since the Posix epoch (ie, what time()
// would return on a properly configured Posix system).  For a simple server that can do this, see
// `../server`.
//
// TODO: Issue 23: This is hacky, it can be integrated with Posix time code by using settimeofday() after
// obtaining the time base.

#include "snappytime.h"
#include "config.h"
#include "device.h"
#include "log.h"
#include "network.h"
#include "util.h"

#ifdef TIMESERVER

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

static bool time_configured;

static bool configure_time() {
  if (time_configured) {
    return true;
  }

  // TODO: These error paths must properly spin down the network!
  auto holder = connect_to_wifi();
  if (!holder.is_valid()) {
    return false;
  }
  WiFiClient wifiClient;
  HTTPClient httpClient;
  // GET /time returns a number, representing the number of seconds UTC since the start
  // of the Posix epoch.
  if (!httpClient.begin(wifiClient, time_server_host(), time_server_port(), "/time")) {
    return false;
  }
  int retval = httpClient.GET();
  if (retval < 200 || retval > 299) {
    return false;
  }
  unsigned long timebase;
  if (sscanf(httpClient.getString().c_str(), "%lu", &timebase) != 1) {
    return false;
  }
  configure_clock(timebase);
  httpClient.end();
  wifiClient.stop();
  time_configured = true;
  return true;
}

ConfigureTimeTask* ConfigureTimeTask::handle;

void ConfigureTimeTask::execute(SnappySenseData*) {
  static unsigned long backoff = 60*1000;
  if (!configure_time()) {
    log("Failed to configure time - connection or protocol error.  Will try later.\n");
    sched_microtask_after(this, backoff);
    if (backoff < 60*60*1000) {
      backoff *= 2;
    }
    return;
  }
  log("Successfully configured time\n");
  handle = nullptr;
}

#endif // TIMESERVER
