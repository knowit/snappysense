// Connectivity management

#ifndef network_h_included
#define network_h_included

#include "main.h"

#ifdef SNAPPY_WIFI

// Call this before anything else.
void wifi_init();

// This will attempt to connect to one of the configured access points.  This may require
// retries; if a retry is needed, it posts EvCode::COMM_WIFI_CLIENT_RETRY to the main queue.
// In the end, the main queue receives COMM_WIFI_CLIENT_FAILED or COMM_WIFI_CLIENT_UP.
void wifi_enable_start();

// Called from main thread in response to COMM_WIFI_CLIENT_RETRY messages.
void wifi_enable_retry();

// This can be called after connection has succeeded or while we're trying to connect, and
// will kill connection attempts.  It will not post any messages, but it cannot remove any
// messages already posted so the main thread must contend with the possibility of seeing
// COMM_WIFI_CLIENT_RETRY, COMM_WIFI_CLIENT_FAILED, and COMM_WIFI_CLIENT_UP messages after
// the wifi client has been turned off.
void wifi_disable();

// Create a soft access point with the given ssid and password (password can be nullptr).
// If successful, return true and assign *ip; otherwise return false.  If false is returned
// it's probably pointless to retry the operation.
bool wifi_create_access_point(const char* ssid, const char* password, IPAddress* ip);

// Returns an empty string if we're not connected, otherwise the IP address from the WiFi
// object.  Works in both client and AP modes.
String wifi_local_ip();

#endif // SNAPPY_WIFI

#endif // !network_h_included
