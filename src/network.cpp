// Connectivity management

#include "network.h"
#include "config.h"
#include "log.h"

#include <WiFi.h>
#include <WiFiAP.h>

static unsigned refcount = 0;

void WiFiHolder::incRef() {
  refcount++;
}

void WiFiHolder::decRef() {
  --refcount;
  if (refcount == 0) {
    log("WiFi: Bringing down network\n");
    WiFi.disconnect();
  }
}

WiFiHolder::WiFiHolder(bool did_create) : valid(did_create) {
  if (valid) {
    incRef();
  }
  log("WiFi: refcount = %d\n", refcount);
}

WiFiHolder::WiFiHolder(const WiFiHolder& other) {
  if (other.valid) {
    incRef();
  }
  valid = other.valid;
  log("WiFi: refcount = %d\n", refcount);
}

WiFiHolder& WiFiHolder::operator=(const WiFiHolder& other) {
  if (other.valid && !valid) {
    incRef();
  } else if (!other.valid && valid) {
    decRef();
  }
  valid = other.valid;
  log("WiFi: refcount = %d\n", refcount);
  return *this;
}

WiFiHolder::~WiFiHolder() {
  if (valid) {
    decRef();
  }
  log("WiFi: refcount = %d\n", refcount);
}

WiFiHolder connect_to_wifi() {
  if (refcount > 0) {
    return WiFiHolder(true);
  }

  // Connect to local WiFi network
  // FIXME: Failure conditions need to be checked and reported
  WiFi.begin(access_point_ssid(), access_point_password());
  log("WiFi: Connecting to network ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    log(".");
  }
  log("\n");
  // Create the holder first: otherwise local_ip_address() will return an empty string.
  WiFiHolder holder(true);
  log("Connected. Device IP address: %s\n", local_ip_address().c_str());
  return holder;
}

String local_ip_address() {
  if (refcount == 0) {
    return String();
  }
  return WiFi.localIP().toString();
}
