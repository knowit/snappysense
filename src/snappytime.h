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

time_t get_time();
struct tm snappy_local_time();
String format_time(const struct tm& time);

class ConfigureTimeTask final : public MicroTask {
public:
  const char* name() override {
    return "configure time";
  }
  void execute(SnappySenseData*) override;
};
#endif

#endif // !snappytime_h_included
