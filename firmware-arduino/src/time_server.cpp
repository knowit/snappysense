// Interaction with an ntp server; configuring the time.

#include "time_server.h"

#ifdef SNAPPY_NTP

#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "config.h"
#include "log.h"

struct TimeServerState {
  TimeServerState() : timeClient(ntpUDP) {}
  WiFiUDP ntpUDP;
  NTPClient timeClient;
  bool first_time = true;
};

static TimeServerState* timeserver_state;
static TimerHandle_t timeserver_timer;

// Set to true if we have received a time from the time server and have set the
// system clock.
static bool time_configured;

// When time_configured is true, this is the number of seconds that was added to the
// clock at the time when time was adjusted.  This can be used to adjust time readings
// that were made before time was adjusted.
static time_t time_adjust;

static void put_delayed_retry() {
  xTimerStart(timeserver_timer, portMAX_DELAY);
}

static void configure_clock(time_t t) {
  time_t now = time(nullptr);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  time_adjust = t - now;
  time_configured = true;
  log("Time adjustment %u\n", (unsigned)time_adjust);
}

static void maybe_configure_time() {
  if (!timeserver_state->first_time) {
    // FIXME: this function just discards the error code, no way to check
    timeserver_state->timeClient.begin();
  }
  put_main_event(EvCode::COMM_ACTIVITY);
  // FIXME: update() blocks, this is not what we want.
  if (timeserver_state->timeClient.update()) {
    log("Time configured\n");
    // FIXME: There's a subtle source of bugs here.  I've observed once that the
    // time returned is the current time.  In that case the time_adjust will become
    // zero, which will be interpreted by the mqtt code as "hold the message for later",
    // and it will never get out of that state.  Probably we should have a fail-safe
    // here where, if the epoch time is is below some known cutoff, we either
    // retry later, or we use the cutoff as the current time.
    configure_clock(timeserver_state->timeClient.getEpochTime());
  } else {
    put_delayed_retry();
  }
}

time_t time_adjustment() {
  if (time_configured) {
    return time_adjust;
  }
  return 0;
}

void ntp_init() {
  // We retry every 10s through the comm window if we can't get a connection.
  timeserver_timer = xTimerCreate("time server", pdMS_TO_TICKS(ntp_retry_s() * 1000), pdFALSE, nullptr,
                                  [](TimerHandle_t){ put_main_event(EvCode::COMM_NTP_WORK); });
}

bool ntp_have_work() {
  if (!time_configured) {
    return true;
  }
  if (timeserver_state != nullptr) {
    return true;
  }
  return false;
}

void ntp_start() {
  if (time_configured) {
    return;
  }
  log("Attempting to configure time\n");
  assert(timeserver_state == nullptr);
  timeserver_state = new TimeServerState;
  maybe_configure_time();
}

// Called from the main loop in response to COMM_NTP_WORK messages.
void ntp_work() {
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
void ntp_stop() {
  delete timeserver_state;
  timeserver_state = nullptr;
  xTimerStop(timeserver_timer, portMAX_DELAY);
}

#endif // SNAPPY_NTP
