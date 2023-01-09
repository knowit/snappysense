#ifndef control_task_h_included
#define control_task_h_included

#include "main.h"
#include "sensor.h"

// A control task is run by the main loop on behalf of some subsystem.  It is enqueued
// with run_control_task(), which takes ownership of the object being passed.  Once
// the task has been run, the object is deleted.
//
// The central point of this is that random subsystems should not be manipulating random
// other parts of the system, for example, the command processor should not be directly
// asking the device to read new data, and mqtt should not manipulating actuators or the
// display.  These are all performed with only the main loop on the stack.
//
// The virtual methods declared in the tasks are all defined in main.cpp.

class ControlTask {
public:
  // Perform the action
  virtual void execute(SnappySenseData*) = 0;

  // This is the deadline for the task in terms of millis().  A deadline of 0
  // means "run right away", and any deadline before now will have the same effect.
  virtual unsigned long deadline() {
    return 0;
  }

  // Private to the scheduler
  ControlTask* next = nullptr;
};

extern void run_control_task(ControlTask* t);

// The following are used only by MQTT at the moment but there's nothing really
// MQTT-specific about them.

#ifdef MQTT_UPLOAD

// A task that enables or disables the device

class ControlEnableTask : public ControlTask {
  bool flag;
public:
  ControlEnableTask(bool flag) : flag(flag) {}
  void execute(SnappySenseData*) override;
};

// A task that sets the mqtt capture interval.

class ControlSetIntervalTask : public ControlTask {
  unsigned interval_s;
public:
  ControlSetIntervalTask(unsigned interval_s) : interval_s(interval_s) {}
  void execute(SnappySenseData*) override;
};

// A task that interacts with an actuator to change an environmental factor.

class ControlActuatorTask : public ControlTask {
  String actuator;
  double reading;
  double ideal;
public:
  ControlActuatorTask(String&& actuator, double reading, double ideal)
    : actuator(std::move(actuator)), reading(reading), ideal(ideal)
  {}
  void execute(SnappySenseData*) override;
};
#endif // MQTT_UPLOAD

class ControlReadSensorsTask : public ControlTask {
public:
  void execute(SnappySenseData*) override;
};

#endif // !control_task_h_included
