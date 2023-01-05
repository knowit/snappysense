// Connectivity management

#include "network.h"
#include "config.h"
#include "log.h"

#include <WiFi.h>
#include <WiFiAP.h>

static unsigned refcount = 0;

WiFiHolder::WiFiHolder(bool did_create) : valid(did_create) {}

WiFiHolder::WiFiHolder(const WiFiHolder& other) {
  if (other.valid) {
    refcount++;
  }
  valid = other.valid;
}

WiFiHolder& WiFiHolder::operator=(const WiFiHolder& other) {
  if (other.valid && !valid) {
    refcount++;
  }
  valid = other.valid;
  return *this;
}

WiFiHolder::~WiFiHolder() {
  if (valid) {
    if (--refcount) {
      WiFi.disconnect();
    }
  }
}

WiFiHolder connect_to_wifi() {
  // Connect to local WiFi network
  // FIXME: Failure conditions need to be checked and reported
  WiFi.begin(access_point_ssid(), access_point_password());
  log("Connecting to local network ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    log(".");
  }
  WiFiHolder holder(true);
  log(" Connected. Device IP address: %s\n", local_ip_address().c_str());
  return holder;
}

String local_ip_address() {
  if (refcount == 0) {
    return String();
  }
  return WiFi.localIP().toString();
}