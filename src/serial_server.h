// A task that reads from the serial port.

#ifndef serial_server_h_included
#define serial_server_h_included

#include "main.h"

#ifdef SERIAL_SERVER

// Read a line of text from the serial port and when complete, pass it to a dispatch
// function passed as a parameter.  parameters->handle() should not block; it should
// send the input to some other task for any extensive processing.

extern TaskHandle_t serial_input_task_handle;

void serial_input_reader_task(void* parameters /* ReadLineHandler* */);

#endif // SERIAL_SERVER

#endif // !serial_server_h_included
