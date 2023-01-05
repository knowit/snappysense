// Connectivity management

#ifndef network_h_included
#define network_h_included

#include "main.h"

class WiFiHolder {
  bool valid;
public:
  WiFiHolder(bool did_create = false);
  WiFiHolder& operator=(const WiFiHolder& other);
  WiFiHolder(const WiFiHolder& other);
  ~WiFiHolder();
};

WiFiHolder connect_to_wifi();
String local_ip_address();

#endif // !network_h_included
