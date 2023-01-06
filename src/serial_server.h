// An interactive server that reads from the serial port.

#ifndef serial_server_h_included
#define serial_server_h_included

#include "main.h"
#include "sensor.h"

#ifdef SERIAL_SERVER
// Read from the serial port.  This will not block!  If a complete line has been read,
// it is processed by the command processor, which will produce some output on the
// serial line.  If a complete line has not been read the input is buffered until the
// next time the serial line is polled.
void maybe_handle_serial_request(SnappySenseData* data);
#endif

#endif // !serial_server_h_included
