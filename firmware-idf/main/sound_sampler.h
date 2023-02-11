/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef sound_sampler_h_included
#define sound_sampler_h_included

#include "main.h"

#ifdef SNAPPY_READ_NOISE

/* Initialize the sampler subsystem: create a background task, timers, and so on. */
bool sound_sampler_begin() WARN_UNUSED;

/* Start the sound sensor on a background task, compute the sound level continually, send
   EV_SOUND_SAMPLE events on the main event queue about readings every so often.  */
void sound_sampler_start();

/* Stop the sound sensor, report the current reading on the main event queue if pertinent.  */
void sound_sampler_stop();

#endif /* SNAPPY_READ_NOISE */

#endif /* !sound_sampler_h_included */
