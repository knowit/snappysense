// An interactive server that reads from the serial port.

#include "serial_server.h"

#if defined(SERIAL_SERVER) || defined(INTERACTIVE_CONFIGURATION)

#include "command.h"

void ReadSerialInputTask::execute(SnappySenseData*) {
  while (Serial.available() > 0) {
    int ch = Serial.read();
    if (ch <= 0 || ch > 127) {
      // Junk character
      continue;
    }
    if (ch == '\r' || ch == '\n') {
      // End of line (we handle CRLF by handling CR and ignoring blank lines)
      if (line.isEmpty()) {
        continue;
      }
      perform();
      line.clear();
      continue;
    }
    line += (char)ch;
  }
}

#ifdef SERIAL_SERVER
void ReadSerialCommandInputTask::perform() {
  sched_microtask_after(new ProcessCommandTask(line, &Serial), 0);
}
#endif

#endif // SERIAL_SERVER || INTERACTIVE_CONFIGURATION
