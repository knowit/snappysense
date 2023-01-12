// Interactive commands for serial port and web server

#ifndef command_h_included
#define command_h_included

#include "main.h"

#ifdef SNAPPY_COMMAND_PROCESSOR

#include "microtask.h"
#include "sensor.h"

// This is not final, because the web server subclasses it to handle garbage
// collection of the output stream.
class ProcessCommandTask : public MicroTask {
  String cmd;
  Stream* output;
public:
  // It is critical that the output stream live at least as long as this task.
  // But NOTE CAREFULLY that the stream may die when this task dies; if this
  // task spawns other tasks, it may not pass the stream on to those other
  // tasks.
  ProcessCommandTask(String cmd, Stream* output) : cmd(cmd), output(output) {}
  const char* name() override {
    return "Process command";
  }
  void execute(SnappySenseData*) override;
};

#endif // SNAPPY_COMMAND_PROCESSOR

#endif // !command_h_included
