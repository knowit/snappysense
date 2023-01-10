// Support for the device acting as a simple web server, for querying data across HTTP

#ifndef web_server_h_included
#define web_server_h_included

#include "main.h"

#ifdef WEB_SERVER
#include "sensor.h"

struct WebServerState;
struct WebClientState;

class ReadWebInputTask final : public MicroTask {
  WebServerState* state = nullptr;
  void start();
  void poll(WebClientState*);
public:
  const char* name() override {
    return "Web server input";
  }
  void execute(SnappySenseData*) override;
};
#endif

#endif // web_server_h_included
