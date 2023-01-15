// A task that reads from the serial port.

#include "serial_server.h"

#ifdef SERIAL_SERVER

#include "config.h"
#include "log.h"
#include "microtask.h"

// A FreeRTOS task that reads from serial (USB) input and dispatches each line
// to a consumer.  As the serial input is on the FeatherESP and there's only
// one of these tasks there's no need to synchronize access to the serial input.
//
// TODO: #13: Make serial input interrupt-driven, not polling.

void serial_input_reader_task(void* parameters) {
  log("Starting serial server\n");
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
