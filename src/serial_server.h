// An interactive server that reads from the serial port.

#ifndef serial_server_h_included
#define serial_server_h_included

#include "main.h"
#include "microtask.h"

#ifdef SNAPPY_SERIAL_LINE

// Read from the serial port.  This will not block!  If a complete line has been read,
// a task is spun off to process it by the a suitable processor, which will produce some
// output on the serial line.  The processor is defined by the subclass.

class ReadSerialInputTask : public MicroTask {
protected:
  String line;
  virtual void perform() = 0;
public:
  void execute(SnappySenseData*) override;
};

#endif // SNAPPY_SERIAL_LINE

#endif // !serial_server_h_included
