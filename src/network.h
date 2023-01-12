// Connectivity management

#ifndef network_h_included
#define network_h_included

#include "main.h"

#ifdef SNAPPY_WIFI

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

#endif // SNAPPY_WIFI

#endif // !network_h_included
