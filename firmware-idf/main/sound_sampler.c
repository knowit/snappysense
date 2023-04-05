/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Generic sound sampler.  The sampler can be enabled or disabled.  When it is enabled, it will read
   the available sound sampler device, integrate the readings over time, and occasionally report the
   integrated readings on the event queue. */

#include "sound_sampler.h"

#ifdef SNAPPY_READ_NOISE

#include "device.h"

#define SAMPLER_INTERVAL_MS 10

static TimerHandle_t sampler_clock;
static bool sampler_running;
static unsigned sampler_accum;
static bool have_sampled_value;

static void clock_callback(TimerHandle_t t) {
  put_main_event(EV_MEMS_WORK);
}

bool sound_sampler_begin() {
  sampler_clock = xTimerCreate("sampler",
                               pdMS_TO_TICKS(SAMPLER_INTERVAL_MS),
                               pdTRUE,
                               NULL,
                               clock_callback);
  return sampler_clock != NULL;
}

void sound_sampler_start() {
  if (!sampler_running) {
    xTimerStart(sampler_clock, portMAX_DELAY);
    sampler_accum = 0;
    have_sampled_value = false;
    sampler_running = true;
  }
}

void sample_noise() {
  if (sampler_running) {
#ifdef SNAPPY_ADC_SEN0487
    unsigned r = sen0487_sound_level();
#endif
    have_sampled_value = true;
    sampler_accum = r > sampler_accum ? r : sampler_accum;
  }
}

void sound_sampler_stop() {
  if (sampler_running) {
    sampler_running = 0;
    xTimerStop(sampler_clock, portMAX_DELAY);
    if (have_sampled_value) {
      put_main_event_with_ival(EV_MEMS_SAMPLE, sampler_accum);
    }
  }
}

#endif /* SNAPPY_READ_NOISE */
