// An interactive server that reads from the serial port.

#include "serial_server.h"

#ifdef SERIAL_SERVER

#include "config.h"
#include "command.h"

void serial_input_reader_task(void* parameters) {
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
    vTaskDelay(serial_command_poll_interval_s() * 1000 / portTICK_PERIOD_MS );
  }
}

#if 0
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
      sched_microtask_after(new ProcessCommandTask(buf, &Serial), 0);
      buf.clear();
      continue;
    }
    buf += (char)ch;
  }
}
#endif

#endif // SERIAL_SERVER
