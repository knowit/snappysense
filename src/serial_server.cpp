// An interactive server that reads from the serial port.

#include "serial_server.h"
#include "command.h"

#ifdef SERIAL_SERVER

void maybe_handle_serial_request(SnappySenseData* data) {
    static String buf;
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
            process_command(data, buf, &Serial);
            buf.clear();
            continue;
        }
        buf += (char)ch;
    }
}

#endif // SERIAL_SERVER
