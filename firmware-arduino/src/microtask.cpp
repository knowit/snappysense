// Lightweight non-preemptive tasking system

#include "microtask.h"
#include "config.h"
#include "log.h"

//#define MICROTASK_LOGGING

// FIXME: Issue 16: millis() overflows after 49 days.  We could just reboot?  And if
// we enter deep-sleep state everything may be handled differently in any case.

// The task_queue is sorted by increasing deadline.  During system initialization
// we can add tasks directly to the queue, but during task processing this
// must be done more carefully to avoid upsetting the queue structure.

static MicroTask* task_queue;

// The blocked_queue has tasks all with deadline 0, so it is sorted FIFO by
// add_task.

static MicroTask* blocked_queue;

static void add_task(MicroTask** task_queue, MicroTask* task) {
  MicroTask *prev = nullptr;
  MicroTask *curr = *task_queue;
  while (curr != nullptr && curr->deadline_millis <= task->deadline_millis) {
    prev = curr;
    curr = curr->next;
  }
  task->next = curr;
  if (prev == nullptr) {
    *task_queue = task;
  } else {
    prev->next = task;
  }
}

void sched_microtask_after(MicroTask* task, unsigned long delay_ms) {
  task->periodic = false;
  task->rescheduled = true;
  task->deadline_millis = millis() + delay_ms;
  add_task(&task_queue, task);
}

void sched_microtask_periodically(MicroTask* task, unsigned long interval_ms) {
  task->periodic = true;
  task->deadline_millis = millis();
  task->delay_ms = interval_ms;
  add_task(&task_queue, task);
}

static void schedule_unblocked_tasks() {
  MicroTask* que = blocked_queue;
  MicroTask* prev = nullptr;
  while (que != nullptr) {
    if (!que->blocked) {
      MicroTask* it = que;
      que = que->next;
      if (prev != nullptr) {
        prev->next = que;
      } else {
        blocked_queue = que;
      }
      it->deadline_millis = millis();
      add_task(&task_queue, it);
    } else {
      prev = que;
      que = que->next;
    }
  }
}

unsigned long run_scheduler(SnappySenseData* data) {
  // Make sure all runnable tasks are seen
  schedule_unblocked_tasks();

  // Loop while there are runnable tasks.
  while (task_queue != nullptr && millis() >= task_queue->deadline_millis) {
    // Slice off the runnable tasks to avoid confusing the queue.
    MicroTask* runnable = task_queue;
    MicroTask* curr = runnable;
    MicroTask* prev = nullptr;
    unsigned long now = millis();
    while (curr != nullptr && now >= curr->deadline_millis) {
      prev = curr;
      curr = curr->next;
    }
    if (prev != nullptr) {
      prev->next = nullptr;
    }
    task_queue = curr;

    // Run the runnable tasks.
    while (runnable != nullptr) {
      auto* it = runnable;
      runnable = runnable->next;
      it->next = nullptr;
      it->rescheduled = false;
      if (it->blocked) {
        // Do nothing yet
#ifdef MICROTASK_LOGGING
        log("Task %s not runnable: blocked\n", it->name());
#endif
      } else if (device_enabled() || !it->only_when_device_enabled()) {
#ifdef MICROTASK_LOGGING
        log("Running task %s\n", it->name());
#endif
        it->execute(data);
#ifdef MICROTASK_LOGGING
        log("Ran task %s\n", it->name());
#endif
      } else {
#ifdef MICROTASK_LOGGING
        log("Task %s not runnable: device disabled\n", it->name());
#endif
      }
      if (it->periodic) {
        // Here, it->blocked could mean that it was blocked before we tried to
        // execute it or it blocked itself while executing.  Either way, it was
        // on the run queue so now it needs to go on the blocked queue
        if (it->blocked) {
          it->deadline_millis = 0;
          add_task(&blocked_queue, it);
        } else {
          it->deadline_millis = millis() + it->delay_ms;
          add_task(&task_queue, it);
        }
      } else if (it->rescheduled) {
        // Nothing - it's already on the queue with its new deadline
      } else {
        delete it;
      }
    }

    // Make sure we check to see if tasks became unblocked
    schedule_unblocked_tasks();
  }

  // Compute the sleep time.
  unsigned long next_deadline = ULONG_MAX;
  if (task_queue != nullptr) {
    next_deadline = min(next_deadline, task_queue->deadline_millis);
  }

  if (next_deadline == ULONG_MAX) {
    // The system is completely idle.  This really should not happen because
    // some periodic tasks should always be schedulable and will always have new
    // deadlines even if the sensors are disabled, but I guess idleness could
    // happen during some kinds of testing or experimentation.  Just wait 1s.
    log("System is completely idle.\n");
    return 1000;
  }

  unsigned long now = millis();
  if (next_deadline <= now) {
    return 0;
  }
#ifdef MICROTASK_LOGGING
  log("Waiting %u\n", (next_deadline - now));
#endif
  return next_deadline - now;
}

void block_microtask(MicroTask* t) {
  if (t->periodic) {
    t->blocked = true;
  }
}

void unblock_microtask(MicroTask* t) {
  t->blocked = false;
}
