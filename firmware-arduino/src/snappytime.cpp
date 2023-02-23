// Stuff to obtain the current time from an ad-hoc web server and configure the clock.
//
// The remote server must know how to handle GET to /time; it must respond with a payload that is
// the decimal encoding of the number of seconds elapsed since the Posix epoch (ie, what time()
// would return on a properly configured Posix system).  For a simple server that can do this, see
// `../server`.

#include "snappytime.h"
#include "config.h"
#include "log.h"

#ifdef TIMESERVER

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

struct TimeServerState {
  WiFiClient wifiClient;
  HTTPClient httpClient;
};

static TimeServerState* timeserver_state;
static TimerHandle_t timeserver_timer;
static bool time_configured;

static void put_delayed_retry() {
  xTimerStart(timeserver_timer, portMAX_DELAY);
}

void configure_clock(time_t t) {
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
}

static void maybe_configure_time() {
  // GET /time returns a number, representing the number of seconds UTC since the start
  // of the Posix epoch.
  if (!timeserver_state->httpClient.begin(timeserver_state->wifiClient, time_server_host(), time_server_port(), "/time")) {
    put_delayed_retry();
    return;
  }
  put_main_event(EvCode::COMM_ACTIVITY);
  int retval = timeserver_state->httpClient.GET();
  if (retval < 200 || retval > 299) {
    // The server seems dead, so no sense in retrying now.  Retry in next comm window.
    // timeserver_stop() will clean up.
    log("Time configuration failed: server rejected\n");
    return;
  }
  unsigned long timebase;
  if (sscanf(timeserver_state->httpClient.getString().c_str(), "%lu", &timebase) != 1) {
    log("Time configuration failed: bogus time from server\n");
    // As above
    return;
  }
  log("Time configured\n");
  configure_clock(timebase);
  timeserver_state->httpClient.end();
  timeserver_state->wifiClient.stop();
  time_configured = true;
  // timeserver_stop will clean up the state
}

void timeserver_init() {
  // We retry every 10s through the comm window if we can't get a connection.
  timeserver_timer = xTimerCreate("time server", pdMS_TO_TICKS(time_server_retry_s() * 1000), pdFALSE, nullptr,
                                  [](TimerHandle_t){ put_main_event(EvCode::COMM_TIMESERVER_WORK); });
}

bool timeserver_have_work() {
  if (!time_configured) {
    return true;
  }
  if (timeserver_state != nullptr) {
    return true;
  }
  return false;
}

void timeserver_start() {
  if (time_configured) {
    return;
  }
  log("Attempting to configure time\n");
  assert(timeserver_state == nullptr);
  timeserver_state = new TimeServerState;
  maybe_configure_time();
}

// Called from the main loop in response to COMM_TIMESERVER_WORK messages.
void timeserver_work() {
  if (time_configured) {
    return;
  }
  if (timeserver_state == nullptr) {
    // Comm window was closed already, this is just a spurious callback
    return;
  }
  maybe_configure_time();
}

// Stop trying to connect to the time server, if that's still going on.
void timeserver_stop() {
  delete timeserver_state;
  timeserver_state = nullptr;
  xTimerStop(timeserver_timer, portMAX_DELAY);
}

#endif // TIMESERVER
