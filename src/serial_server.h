// An interactive server that reads from the serial port.

#ifndef serial_server_h_included
#define serial_server_h_included

#include "main.h"

#ifdef SERIAL_SERVER

#include "microtask.h"
#include "sensor.h"

// Read from the serial port.  This will not block!  If a complete line has been read,
// a task is spun off to process it by the command processor, which will produce some
// output on the serial line.
class ReadSerialInputTask : public MicroTask {
  String buf;
public:
  const char* name() override {
    return "Serial server input";
  }
  void execute(SnappySenseData*) override;
};

#endif

#endif // !serial_server_h_included
