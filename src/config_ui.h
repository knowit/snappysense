#ifndef config_ui_h_included
#define config_ui_h_included

#include "main.h"
#include "serial_input.h"
#include "microtask.h"
#include "util.h"
#include "web_server.h"

#ifdef SERIAL_CONFIGURATION
class SerialConfigTask final : public ReadSerialInputTask {
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
class WebConfigRequestHandler final : public WebRequestHandler {
public:
  WebConfigRequestHandler(WiFiClient&& client) : WebRequestHandler(std::move(client)) {}
  virtual void process_request() override;
  virtual void failed_request() override;
};

class WebConfigTask final : public WebInputTask {
public:
  WebRequestHandler* create_request_handler(WiFiClient&& client) override {
    return new WebConfigRequestHandler(std::move(client));
  }

  bool start() override;

  const char* name() override {
    return "Web server config input";
  }
};
#endif // WEB_CONFIGURATION
#endif // !config_ui_h_included
