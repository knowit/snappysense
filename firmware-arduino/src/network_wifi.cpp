// Connectivity management

#include "network_wifi.h"

#ifdef SNAPPY_WIFI

#include <WiFi.h>
#include <WiFiAP.h>
#include "config.h"
#include "device.h"
#include "log.h"
#include "slideshow.h"

/* Server/Client wifi state machine is basically similar to the Unix stack:
 *  WiFi.begin() connects the system to a local access point.
 *  Then, create a WiFiServer `server` to handle incoming connections on a port.
 *  server.begin() is bind() + listen(): it starts listening on a port, but does not block.
 *  server.available() is accept() + client socket setup, but it will not block: if there is no client,
 *       available() returns a falsy client.  Be sure not to spin...  A callback would have been better.
 *       Anyway, I can't tell if the server blocks subsequent connects until we've accepted,
 *       or overwrites one connect with a new one, but perhaps it doesn't matter.
 *  client.connected() can become false at any time if the client disconnects.
 *  client.available() says there's data.  Again, would be better to get a callback.
 *  client.stop() closes the client socket after use
 *  server.close() closes the server socket and stops listening
 */

static int num_access_points_tried = 0;
static int current_access_point = 0;

static constexpr int MAX_TIMEOUTS = 10;
static int num_timeouts = 0;

enum class WiFiState {
  STARTING,
  RETRYING,
  CONNECTED,
  FAILED,
  STOPPED
};
static WiFiState wifi_state = WiFiState::STARTING;

static TimerHandle_t retry_timer;

static void put_delayed_retry() {
  xTimerStart(retry_timer, portMAX_DELAY);
}

static void connect_to_wifi() {
again:
  switch (wifi_state) {
    case WiFiState::STARTING: {
      // We're in STARTING every time we try a new AP.  Once we've tried all APs we're done.
      if (num_access_points_tried == 3) {
        wifi_state = WiFiState::FAILED;
        put_main_event(EvCode::COMM_WIFI_CLIENT_FAILED);
        WiFi.disconnect(true);
        log("WiFi: Failed to connect to any access point\n");
        return;
      }
      // Next AP.
      const char* ap = access_point_ssid(current_access_point+1);
      const char* pw = access_point_password(current_access_point+1);
      num_access_points_tried++;
      if (*ap == 0) {
        goto again;
      }
      if (*pw == 0) {
        pw = nullptr;
      }
      num_timeouts = 0;
      log("Trying access point: [%s]\n", ap);
      WiFi.begin(ap, pw);
      put_delayed_retry();
      wifi_state = WiFiState::RETRYING;
      return;
    }
    case WiFiState::RETRYING: {
      if (WiFi.status() == WL_CONNECTED) {
        persistent_data.network_wifi.last_successful_access_point = current_access_point;
        wifi_state = WiFiState::CONNECTED;
        put_main_event(EvCode::COMM_WIFI_CLIENT_UP);
        log("WiFi: Connected. Device IP address: %s\n", wifi_local_ip().c_str());
        return;
      }
      if (num_timeouts == MAX_TIMEOUTS) {
        current_access_point = (current_access_point + 1) % 3;
        wifi_state = WiFiState::STARTING;
        goto again;
      }
      num_timeouts++;
      put_delayed_retry();
      return;
    }
    case WiFiState::FAILED:
    case WiFiState::STOPPED:
    case WiFiState::CONNECTED:
      return;
    default:
      panic("Should not happen");
  }
}

void wifi_init() {
  retry_timer = xTimerCreate("wifi retry", pdMS_TO_TICKS(wifi_retry_ms()), pdFALSE, nullptr,
                             [](TimerHandle_t) { put_main_event(EvCode::COMM_WIFI_CLIENT_RETRY); });
}

void wifi_enable_start() {
  num_access_points_tried = 0;
  current_access_point = persistent_data.network_wifi.last_successful_access_point;
  wifi_state = WiFiState::STARTING;
  connect_to_wifi();
}

void wifi_enable_retry() {
  connect_to_wifi();
}

void wifi_disable() {
  switch (wifi_state) {
    case WiFiState::RETRYING:
    case WiFiState::CONNECTED:
      log("WiFi: Disconnected\n");
      WiFi.disconnect(true);
      break;
    default:
      break;
  }
  wifi_state = WiFiState::STOPPED;
}

String wifi_local_ip() {
  if (wifi_state != WiFiState::CONNECTED) {
    return String();
  }
  return WiFi.localIP().toString();
}

bool wifi_create_access_point(const char* ssid, const char* password, IPAddress* ip) {
  if (!WiFi.softAP(ssid, password)) {
    return false;
  }
  wifi_state = WiFiState::CONNECTED;
  *ip = WiFi.softAPIP();
  log("Soft AP SSID %s, IP address: %s\n", ssid, ip->toString().c_str());
  return true;
}

#endif // SNAPPY_WIFI
