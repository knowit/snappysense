// Connectivity management

#include "network.h"

#ifdef SNAPPY_WIFI

#include "config.h"
#include "device.h"
#include "log.h"
#include "slideshow.h"

#include <WiFi.h>
#include <WiFiAP.h>

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

//#define REFCOUNT_LOGGING
#define WIFI_LOGGING

static unsigned refcount = 0;

void WiFiHolder::incRef() {
  refcount++;
#ifdef REFCOUNT_LOGGING
  log("WiFi: refcount = %d\n", refcount);
#endif
}

void WiFiHolder::decRef() {
  --refcount;
#ifdef REFCOUNT_LOGGING
  log("WiFi: refcount = %d\n", refcount);
#endif
  if (refcount == 0) {
#ifdef WIFI_LOGGING
    log("WiFi: Bringing down network\n");
#endif
    WiFi.disconnect();
  }
}

WiFiHolder::WiFiHolder(bool did_create) : valid(did_create) {
  if (valid) {
    incRef();
  }
}

WiFiHolder::WiFiHolder(const WiFiHolder& other) {
  if (other.valid) {
    incRef();
  }
  valid = other.valid;
}

WiFiHolder& WiFiHolder::operator=(const WiFiHolder& other) {
  if (other.valid && !valid) {
    incRef();
  } else if (!other.valid && valid) {
    decRef();
  }
  valid = other.valid;
  return *this;
}

WiFiHolder::~WiFiHolder() {
  if (valid) {
    decRef();
  }
}

WiFiHolder connect_to_wifi() {
  // TODO: If we fail to bring up the network, we should wait some time before trying again,
  // otherwise we're just wasting energy.
  if (refcount > 0) {
    return WiFiHolder(true);
  }

  static int last_successful_access_point = 0;
  int access_point = last_successful_access_point;
  bool is_connected = false;
  for (int i=0 ; i < 3; i++) {
    const char* ap = access_point_ssid(access_point+1);
    const char* pw = access_point_password(access_point+1);
    if (*ap == 0) {
      continue;
    }
    if (*pw == 0) {
      pw = nullptr;
    }
    log("Trying access point: [%s]\n", ap);
    wl_status_t stat = WiFi.begin(ap, pw);
    int attempts = 0;
    while (stat != WL_CONNECTED && attempts < 5) {
      // TODO: Embedded delay
      delay(500);
      stat = WiFi.status();
      attempts++;
    }
    if (stat == WL_CONNECTED) {
      last_successful_access_point = access_point;
      is_connected = true;
      break;
    }
    access_point = (access_point + 1) % 3;
  }
  if (is_connected) {
    WiFiHolder holder(true);
#ifdef SLIDESHOW_MODE
    if (SlideshowTask::handle) {
      SlideshowTask::handle->setWiFiStatus(true);
    }
#endif
    log("WiFi: Connected. Device IP address: %s\n", local_ip_address().c_str());
    return holder;
  }
  log("WiFi: Failed to connect to any access point\n");
#ifdef SLIDESHOW_MODE
  if (SlideshowTask::handle) {
    SlideshowTask::handle->setWiFiStatus(false);
  }
#else
  render_text("No WiFi\n");
  // TODO: Embedded delay
  delay(1000);
#endif
  return WiFiHolder();
}

String local_ip_address() {
  if (refcount == 0) {
    return String();
  }
  return WiFi.localIP().toString();
}

bool create_wifi_soft_access_point(const char* ssid, const char* password, IPAddress* ip) {
  if (!WiFi.softAP(ssid, password)) {
    return false;
  }
  *ip = WiFi.softAPIP();
  log("Soft AP SSID %s, IP address: %s\n", ssid, ip->toString().c_str());
  return true;
}

#endif // SNAPPY_WIFI
