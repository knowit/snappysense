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

static void put_delayed_retry() {
  xTimerStart(timeserver_timer, portMAX_DELAY);
}

static void configure_clock(time_t t) {
  time_t now = time(nullptr);
  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
  settimeofday(&tv, nullptr);
  persistent_data.time_server.time_adjust = t - now;
  persistent_data.time_server.time_configured = true;
  log("Time adjustment %u\n", (unsigned)persistent_data.time_server.time_adjust);
}

static bool maybe_configure_time() {
  if (!timeserver_state->first_time) {
    // FIXME: this function just discards the error code, no way to check
    timeserver_state->timeClient.begin();
  }
  put_main_event(EvCode::COMM_ACTIVITY);
  // FIXME: update() blocks, this is not what we want.
  if (timeserver_state->timeClient.update()) {
    time_t t = timeserver_state->timeClient.getEpochTime();
    // Bad if it is not between 28 March 2023 and 1 January 2038.
    // Usually this is a spurious error so we will retry later.
    if (t < 1680000000 || t > 2145916800) {
      log("Time configuration error, will retry later\n");
      return false;
    }
    log("Time configured\n");
    configure_clock(t);
  } else {
    put_delayed_retry();
  }
  return true;
}

time_t time_adjustment() {
  if (persistent_data.time_server.time_configured) {
    return persistent_data.time_server.time_adjust;
  }
  return 0;
}

void ntp_init() {
  // We retry every 10s through the comm window if we can't get a connection.
  timeserver_timer = xTimerCreate("time server", pdMS_TO_TICKS(ntp_retry_s() * 1000), pdFALSE, nullptr,
                                  [](TimerHandle_t){ put_main_event(EvCode::COMM_NTP_WORK); });
}

bool ntp_have_work() {
  if (!persistent_data.time_server.time_configured) {
    return true;
  }
  if (timeserver_state != nullptr) {
    return true;
  }
  return false;
}

void ntp_start() {
  if (persistent_data.time_server.time_configured) {
    return;
  }
  log("Attempting to configure time\n");
  assert(timeserver_state == nullptr);
  timeserver_state = new TimeServerState;
  if (!maybe_configure_time()) {
    // We will retry during the next comm window
    ntp_stop();
  }
}

// Called from the main loop in response to COMM_NTP_WORK messages.
void ntp_work() {
  if (persistent_data.time_server.time_configured) {
    return;
  }
  if (timeserver_state == nullptr) {
    // Comm window was closed already, this is just a spurious callback
    return;
  }
  if (!maybe_configure_time()) {
    // We will retry during the next comm window
    ntp_stop();
  }
}

// Stop trying to connect to the time server, if that's still going on.
void ntp_stop() {
  delete timeserver_state;
  timeserver_state = nullptr;
  xTimerStop(timeserver_timer, portMAX_DELAY);
}

#endif // SNAPPY_NTP
