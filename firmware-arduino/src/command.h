// Interactive commands

#ifndef command_h_included
#define command_h_included

#include "main.h"

#ifdef SNAPPY_COMMAND_PROCESSOR

#include "sensor.h"

// Evaluate `cmd`, possibly using the `data` to do so, writing to `out`.
void command_evaluate(const String& cmd, const SnappySenseData& data, Stream& out);

#endif // SNAPPY_COMMAND_PROCESSOR

#endif // !command_h_included
