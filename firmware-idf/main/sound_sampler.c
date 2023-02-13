/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Generic sound sampler.  The sampler can be enabled or disabled.  When it is enabled, it will read
   the available sound sampler device, integrate the readings over time, and occasionally report the
   integrated readings on the event queue. */

#include "sound_sampler.h"

#ifdef SNAPPY_READ_NOISE

#include "device.h"

typedef enum {
  SAMP_EV_START,
  SAMP_EV_STOP,
  SAMP_EV_TICK
} sampler_event_t;

#define SAMPLER_INTERVAL_MS 10

static TimerHandle_t sampler_clock = NULL;
static QueueHandle_t/*<snappy_event_t>*/ evt_queue;
static QueueHandle_t/*<sampler_event_t>*/ sampler_queue;
static bool sampler_running = false;
static unsigned sampler_accum = 0;
static bool have_sampled_value = false;

static void sampler_task(void* arg) {
  for (;;) {
    sampler_event_t ev;
    if (xQueueReceive(sampler_queue, &ev, portMAX_DELAY)) {
      switch (ev) {
      case SAMP_EV_START:
	if (!sampler_running) {
	  /* set up a timer posting clock ticks to the queue */
	  sampler_accum = 0;
	  have_sampled_value = false;
	  sampler_running = true;
	}
	break;
      case SAMP_EV_STOP:
	if (sampler_running) {
	  /* stop the timer */
	  sampler_running = 0;
	  if (have_sampled_value) {
	    snappy_event_t ev = EV_SOUND_SAMPLE | (sampler_accum << 4);
	    xQueueSend(evt_queue, &ev, portMAX_DELAY);
	  }
	}
	break;
      case SAMP_EV_TICK:
	if (sampler_running) {
#ifdef SNAPPY_ADC_SEN0487
	  unsigned r = sen0487_sound_level();
#endif
	  have_sampled_value = true;
	  sampler_accum = r > sampler_accum ? r : sampler_accum;
	  /* Maybe report the value on evt_queue and clear the sampler */
	  /* Maybe report once per second, we can read the clock here or we can have
	     another clock to drive the reporting */
	  /* Restart clock */
	}
	break;
      }
    } else {
      /* Timeout; just repeat the process */
    }
  }
}

static void clock_callback(TimerHandle_t t) {
  sampler_event_t ev = SAMP_EV_TICK;
  xQueueSend(sampler_queue, &ev, 0);
}

bool sound_sampler_begin(QueueHandle_t/*<snappy_event_t>*/ evt_queue) {
  sampler_queue = xQueueCreate(1, sizeof(sampler_event_t));
  sampler_clock = xTimerCreate("sampler", pdMS_TO_TICKS(SAMPLER_INTERVAL_MS),
                              /* restart= */ pdFALSE,
                              NULL, clock_callback);
  return xTaskCreate(sampler_task, "sampler", 1024, NULL, SAMPLER_PRIORITY, NULL) == pdPASS;
}

void sound_sampler_start() {
  uint32_t ev = SAMP_EV_START;
  xQueueSend(sampler_queue, &ev, portMAX_DELAY);
}

void sound_sampler_stop() {
  uint32_t ev = SAMP_EV_STOP;
  xQueueSend(sampler_queue, &ev, portMAX_DELAY);
}

#endif /* SNAPPY_READ_NOISE */
