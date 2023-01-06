// Connectivity management

#include "network.h"
#include "config.h"
#include "log.h"

#include <WiFi.h>
#include <WiFiAP.h>

static unsigned refcount = 0;

WiFiHolder::WiFiHolder(bool did_create) : valid(did_create) {
  if (valid) {
    refcount++;
  }
  log("WiFi: refcount = %d\n", refcount);
}

WiFiHolder::WiFiHolder(const WiFiHolder& other) {
  if (other.valid) {
    refcount++;
  }
  valid = other.valid;
  log("WiFi: refcount = %d\n", refcount);
}

WiFiHolder& WiFiHolder::operator=(const WiFiHolder& other) {
  if (other.valid && !valid) {
    refcount++;
  }
  valid = other.valid;
  log("WiFi: refcount = %d\n", refcount);
  return *this;
}

WiFiHolder::~WiFiHolder() {
  if (valid) {
    --refcount;
    if (refcount == 0) {
      log("WiFi: Bringing down network\n");
      WiFi.disconnect();
    }
  }
  log("WiFi: refcount = %d\n", refcount);
}

WiFiHolder connect_to_wifi() {
  if (refcount == 0) {
    // Connect to local WiFi network
    // FIXME: Failure conditions need to be checked and reported
    WiFi.begin(access_point_ssid(), access_point_password());
    log("WiFi: Connecting to network ");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      log(".");
    }
    log(" Connected. Device IP address: %s\n", local_ip_address().c_str());
  }
  return WiFiHolder(true);
}

String local_ip_address() {
  if (refcount == 0) {
    return String();
  }
  return WiFi.localIP().toString();
}