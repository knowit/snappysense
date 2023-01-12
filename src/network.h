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
};

WiFiHolder connect_to_wifi();
String local_ip_address();

#endif // SNAPPY_WIFI

#endif // !network_h_included
