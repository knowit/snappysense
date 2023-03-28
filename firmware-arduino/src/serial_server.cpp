// An server that reads from the serial port.
//
// TODO: This is almost indistinguishable from the I2C slave input logic,
// it might be useful to merge them.

#include "serial_server.h"

#ifdef SNAPPY_SERIAL_INPUT

#include "config.h"

static String line;
static TimerHandle_t serial_timer;

void serial_server_init() {
  serial_timer = xTimerCreate("serial",
                              pdMS_TO_TICKS(serial_server_poll_interval_ms()),
                              pdTRUE,
                              nullptr,
                              [](TimerHandle_t t) {
                                put_main_event(EvCode::SERIAL_SERVER_POLL);
                              });
}

void serial_server_start() {
  xTimerStart(serial_timer, portMAX_DELAY);
}

void serial_server_stop() {
  xTimerStop(serial_timer, portMAX_DELAY);
}

void serial_server_poll() {
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
      put_main_event(EvCode::SERIAL_INPUT, new String(line));
      line.clear();
      continue;
    }
    line += (char)ch;
  }
}

#endif // SNAPPY_SERIAL_INPUT
