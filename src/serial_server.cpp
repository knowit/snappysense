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
      if (buf.isEmpty()) {
        continue;
      }
      perform();
      buf.clear();
      continue;
    }
    buf += (char)ch;
  }
}

#ifdef SERIAL_SERVER
void ReadSerialCommandInputTask::perform() {
  sched_microtask_after(new ProcessCommandTask(buf, &Serial), 0);
}
#endif

#ifdef INTERACTIVE_CONFIGURATION
void ReadSerialConfigInputTask::perform() {
  // FIXME
  // Unlike command processing, we don't want to fork off a separate task for
  // each line read, because these are not quite independent.  But perhaps the
  // config processing state can be kept in an object shared among the tasks.
  // This would be meaningful since it could then be shared among the different
  // input sources too.
  //    interactive_configuration(&Serial);
  //    enter_end_state("Press reset button!");
  abort();
}
#endif

#endif // SERIAL_SERVER || INTERACTIVE_CONFIGURATION
