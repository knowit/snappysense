// Interactive commands for serial port and web server

#ifndef command_h_included
#define command_h_included

#include "main.h"

#ifdef SNAPPY_COMMAND_PROCESSOR

#include "sensor.h"

void execute_command(Stream* output, String cmd, SnappySenseData* data);

#endif // SNAPPY_COMMAND_PROCESSOR

#endif // !command_h_included
