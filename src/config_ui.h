#ifndef config_ui_h_included
#define config_ui_h_included

#include "main.h"
#include "serial_input.h"
#include "microtask.h"
#include "util.h"

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

#endif // !config_ui_h_included
