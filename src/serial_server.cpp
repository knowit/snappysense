// An interactive server that reads from the serial port.

#include "serial_server.h"

#ifdef SERIAL_SERVER

#include "config.h"
#include "log.h"
#include "microtask.h"

void serial_input_reader_task(void* parameters) {
  log("Entering serial server\n");
  auto* handler = reinterpret_cast<ReadLineHandler*>(parameters);
  String buf;
  for(;;) {
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
        handler->handle(buf);
        buf.clear();
        continue;
      }
      buf += (char)ch;
    }
    delay(serial_command_poll_interval_s() * 1000);
  }
}

#endif // SERIAL_SERVER
