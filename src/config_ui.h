#ifndef config_ui_h_included
#define config_ui_h_included

#include "main.h"
#include "serial_input.h"
#include "microtask.h"
#include "util.h"
#include "web_server.h"

#ifdef INTERACTIVE_CONFIGURATION
class ReadSerialConfigInputTask final : public ReadSerialInputTask {
  enum {
    RUNNING,
    COLLECTING,
  } state = RUNNING;
  List<String> config_lines;
public:
  const char* name() override {
    return "Serial server config input";
  }
  void perform() override;
};
#endif

#ifdef WEB_CONFIGURATION
class WebConfigClient final : public WebClient {
public:
  WebConfigClient(WiFiClient&& client) : WebClient(std::move(client)) {}
  virtual void process_request() override;
  virtual void failed_request() override;
};

class WebConfigTask final : public WebInputTask {
public:
  WebClient* create_client(WiFiClient&& client) override {
    return new WebConfigClient(std::move(client));
  }

  const char* name() override {
    return "Web server config input";
  }
};
#endif // WEB_CONFIGURATION
#endif // !config_ui_h_included
