// Stuff to obtain the current time from a web server and to obtain timestamps with that adjustment.
//
// The remote server must know how to handle GET to /time; it must respond with a payload that is
// the decimal encoding of the number of seconds elapsed since the Posix epoch (ie, what time()
// would return on a properly configured Posix system).  For a simple server that can do this, see
// `../server`.

#ifndef snappytime_h_included
#define snappytime_h_included

#include "main.h"

#ifdef TIMESTAMP
#include "microtask.h"
#include "util.h"

// Return the number of seconds since epoch.
//
// NOTE: If time configuration has not completed then the device clock will
// not have been set and the time will be sometime in January, 1970.

time_t get_time();

// Return the local time (based on get_time(), above) broken out into the
// standard fields.
//
// NOTE: If time configuration has not completed then the device clock will
// not have been set and the time will be sometime in January, 1970.

struct tm snappy_local_time();

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
