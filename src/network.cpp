// Connectivity management

#include "network.h"

#ifdef SNAPPY_WIFI

#include "config.h"
#include "device.h"
#include "log.h"

#include <WiFi.h>
#include <WiFiAP.h>

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
  if (refcount > 0) {
    return WiFiHolder(true);
  }

  // Connect to local WiFi network
  // FIXME: Issue 15: Failure conditions need to be checked and reported but should not block
  // progress per se, we must not hang here.
  // FIXME: Issue 10: There can be multiple accss points, we must scan for one that works.
  // TODO: Issue 21: We should flash a message on the display if no access points work

  static int last_successful_access_point = 0;
  int access_point = last_successful_access_point;
  bool is_connected = false;
  for(int i=0 ; i < 3; i++) {
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
    log("WiFi: Connected. Device IP address: %s\n", local_ip_address().c_str());
    return holder;
  }
  log("WiFi: Failed to connect to any access point\n");
  render_text("No WiFi\n");
  delay(1000);
  return WiFiHolder();
}

String local_ip_address() {
  if (refcount == 0) {
    return String();
  }
  return WiFi.localIP().toString();
}

#endif // SNAPPY_WIFI
