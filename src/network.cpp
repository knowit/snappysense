// Connectivity management

#include "network.h"
#include "config.h"
#include "log.h"

#include <WiFi.h>
#include <WiFiAP.h>

void connect_to_wifi() {
  // Connect to local WiFi network
  // FIXME: Failure conditions need to be checked and reported
  WiFi.begin(access_point_ssid(), access_point_password());
  log("Connecting to local network ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    log(".");
  }
  log(" Connected.  Device IP address: ");
  log(" %s\n", WiFi.localIP().toString());
}

void disconnect_from_wifi() {
    WiFi.disconnect();
}
