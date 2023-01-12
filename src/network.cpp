// Connectivity management

#include "network.h"

#ifdef SNAPPY_WIFI

#include "config.h"
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
  log("Access point: [%s]\n", access_point_ssid(1));
  WiFi.begin(access_point_ssid(1), access_point_password(1));
#ifdef WIFI_LOGGING
  log("WiFi: Bringing up network ");
#endif
  // FIXME: Issue 15: This polling+delay should be implemented as some type of task, so long
  // as the user of connect_to_wifi() can handle that.  I'm seeing a lot of async/await
  // patterns here...
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
#ifdef WIFI_LOGGING
    log(".");
#endif
  }
#ifdef WIFI_LOGGING
  log("\n");
#endif
  // Create the holder first: otherwise local_ip_address() will return an empty string.
  WiFiHolder holder(true);
#ifdef WIFI_LOGGING
  log("WiFi: Connected. Device IP address: %s\n", local_ip_address().c_str());
#endif
  return holder;
}

String local_ip_address() {
  if (refcount == 0) {
    return String();
  }
  return WiFi.localIP().toString();
}

#endif // SNAPPY_WIFI
