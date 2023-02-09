#ifndef config_ui_h_included
#define config_ui_h_included

#include "main.h"
#include "serial_input.h"
#include "microtask.h"
#include "util.h"
#include "web_server.h"

#ifdef WEB_CONFIGURATION
class WebConfigRequestHandler final : public WebRequestHandler {
  bool handle_get_user_config();
  bool handle_show_factory_config();
  bool handle_post_user_config(const char* payload);
  String handle_post_factory_config(const char* payload);
  char* get_post_data();
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
