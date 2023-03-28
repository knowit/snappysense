// A server that reads from the I2C slave port and posts individual lines to
// the event loop.

// TODO: This code is almost indistinguishable from the serial_server code,
// and the web code is not very different also; it might be useful to merge
// some of them.

#include "i2c_server.h"

#ifdef SNAPPY_I2C_SERVER

#include <Wire.h>

static String line;
static TimerHandle_t i2cconfig_timer;

void i2c_server_init() {
  i2cconfig_timer = xTimerCreate("i2cconfig",
                                 pdMS_TO_TICKS(1000),
                                 pdTRUE,
                                 nullptr,
                                 [](TimerHandle_t t) {
                                   put_main_event(EvCode::I2C_SERVER_POLL);
                                 });
}

void i2c_server_start() {
  xTimerStart(i2cconfig_timer, portMAX_DELAY);
}

void i2c_server_stop() {
  xTimerStop(i2cconfig_timer, portMAX_DELAY);
}

void i2c_server_poll() {
  while (Wire1.available() > 0) {
    int ch = Wire1.read();
    if (ch <= 0 || ch > 127) {
      // Junk character
      continue;
    }
    if (ch == '\r' || ch == '\n') {
      // End of line (we handle CRLF by handling CR and ignoring blank lines)
      if (line.isEmpty()) {
        continue;
      }
      put_main_event(EvCode::I2C_INPUT, new String(line));
      line.clear();
      continue;
    }
    line += (char)ch;
  }
}

#endif // SNAPPY_I2C_SERVER
