// An interactive server that reads from the serial port.

#ifndef serial_input_h_included
#define serial_input_h_included

#include "main.h"

#ifdef SNAPPY_SERIAL_INPUT

void serial_server_init();
void serial_server_start();
void serial_server_poll();
void serial_server_stop();

#endif // SNAPPY_SERIAL_INPUT

#endif // !serial_input_h_included
