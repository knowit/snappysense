#ifndef microtask_h_included
#define microtask_h_included

#include "main.h"

// The system is structured around microtasks that are run on top of the Arduino runloop,
// see overview in main.cpp.
//
// The microtask system simplifies the firmware in three ways: it moves decisions about
// what to schedule out of the main loop and into the parts of the system that contain the
// logic for those decisions; it discourages hiding delays down in the subsystems
// where they would just block other things from happening; and it encourages a simple architecture
// where most parts are called directly from the runloop and not ad-hoc from other parts
// of the system.  All in all, the system is more responsive, more modular, and better layered.
//
// At the same time, complexity is added because state that could be kept in local variables
// now needs to be kept in class instances, and those instances need to be managed.
//
// Microtasks are *periodic*, ie, always re-run after a fixed interval after the last run
// finished, or *one-shot*, scheduled at a fixed point in future.  One-shot tasks can
// reschedule themselves and thus become a third type, *aperiodic*.
//
// One-shot tasks that are not rescheduled are deleted by the runloop after having been run.
//
// At this time, tasks are either RUNNING (at most one of these), RUNNABLE (their deadline has
// expired), or WAITING (their deadline is in the future).  No tasks can be BLOCKED waiting
// for an event.  This is a weakness that should be remedied.

struct SnappySenseData;

class MicroTask {
public:
  // These fields and methods are PRIVATE to the scheduler.

  // True for periodic tasks, false for one-shot tasks.
  bool periodic = false;

  // For periodic tasks, this is the interval between the end of one execution and
  // the start of the next.  For one-shot tasks it is currently unused.
  unsigned long delay_ms = 0;

  // When the task is on the run list, the time after which it should be executed.
  unsigned long deadline_millis = 0;

  // Used for the ordered task lists.  The last node in the list has a null value here.
  MicroTask* next = nullptr;

  // This is set by the scheduler to prevent the object from being deleted if it
  // is rescheduled by the execute() function.
  bool rescheduled = false;

public:
  // These methods are for use by derived task types.

  // Frequently subtasks will carry data that require deletion.
  virtual ~MicroTask() {}

  // A unique name for the task type, used for debugging.
  virtual const char* name() = 0;

  // Perform the action.  For a task that is scheduled one-shot, execute() is explicitly
  // allowed to call sched_microtask_after() with the same task and a new deadline, and
  // it will be rescheduled as expected.
  virtual void execute(SnappySenseData*) = 0;

  // Guard: Perform the action only when the device is enabled.
  virtual bool only_when_device_enabled() {
    return false;
  }
};

// Returns the delay, in ms, until the next task should be run.  This should be called
// from the Arduino loop(), which should then delay() as long as the return value indicates
// before returning to its caller.
extern unsigned long run_scheduler(SnappySenseData* data);

// This will schedule the task to be run `delay_ms` from now.
extern void sched_microtask_after(MicroTask* t, unsigned long delay_ms);

// This will schedule the task to be run now, and then periodically `delay_ms` after
// it completes henceforth.
extern void sched_microtask_periodically(MicroTask* t, unsigned long delay_ms);

#endif // !microtask_h_included
