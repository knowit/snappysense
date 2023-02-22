// An interactive server that reads from the serial port.

#include "serial_input.h"

#ifdef SNAPPY_SERIAL_INPUT

#include "command.h"

static String line;

void serial_poll() {
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
      put_main_event(EvCode::PERFORM, new String(line));
      line.clear();
      continue;
    }
    line += (char)ch;
  }
}

#endif // SNAPPY_SERIAL_LINE
