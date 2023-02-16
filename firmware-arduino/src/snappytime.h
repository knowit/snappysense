// Stuff to obtain the current time from a web server and configure the clock.

#ifndef snappytime_h_included
#define snappytime_h_included

#include "main.h"

#ifdef TIMESERVER
#include "microtask.h"
#include "util.h"

// A singleton task that will attempt to contact the time server to obtain the
// current time.  Once it succeeds it will set the device clock to the obtained time.
//
// If it does not succeed it will reschedule itself, with some appropriate backoff,
// to try again at a future time.

class ConfigureTimeTask final : public MicroTask {
public:
  // The handle is set to nullptr once the time configuration task has completed and
  // time has been configured.
  static ConfigureTimeTask* handle;

  ConfigureTimeTask() {
    if (handle != nullptr) {
        panic("The time-task is a singleton");
    }
    handle = this;
  }

  const char* name() override {
    return "configure time";
  }

  void execute(SnappySenseData*) override;
};

#endif

#endif // !snappytime_h_included
