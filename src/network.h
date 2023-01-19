// Connectivity management

#ifndef network_h_included
#define network_h_included

#include "main.h"

#ifdef SNAPPY_WIFI

// WiFiHolder manages the reference count on the WiFi connection.  When the
// reference count drops to 0 the connection goes down.

class WiFiHolder {
  bool valid;
  void incRef();
  void decRef();
public:
  WiFiHolder(bool did_create = false);
  WiFiHolder& operator=(const WiFiHolder& other);
  WiFiHolder(const WiFiHolder& other);
  ~WiFiHolder();
  bool is_valid() const { return valid; }
};

// If the return value tests as !is_valid() then connection failed
// and the connection must not be used.

WiFiHolder connect_to_wifi();

// Returns an empty string if we're not connected.

String local_ip_address();

// Create a soft access point with the given ssid and password (password can be nullptr).
// If successful, return true and assign *ip; otherwise return false.  If false is returned
// it's probably pointless to retry the operation.

bool create_wifi_soft_access_point(const char* ssid, const char* password, IPAddress* ip);

#endif // SNAPPY_WIFI

#endif // !network_h_included
