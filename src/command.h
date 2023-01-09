// Interactive commands for serial port and web server

#ifndef command_h_included
#define command_h_included

#include "main.h"
#include "sensor.h"

#if defined(SERIAL_SERVER) || defined(WEB_SERVER)
// `cmd` is a raw command line coming from serial or web input.  It consists
// of space-delimited words.  The first word is the command; see the end of command.cpp
// for a list of supported commands.  Output from the command processor is written 
// to `output`.  Even failed commands will produce something here.
void process_command(const SnappySenseData& data, const String& cmd, Stream* output);
#endif

#endif
