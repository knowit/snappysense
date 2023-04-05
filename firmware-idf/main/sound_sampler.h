/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef sound_sampler_h_included
#define sound_sampler_h_included

#include "main.h"

#ifdef SNAPPY_READ_NOISE

/* Initialize the sampler subsystem. */
bool sound_sampler_begin() WARN_UNUSED;

/* Start the sound sensor timer, this will place tick events on the main queue causing the
   main thread to call sound_sampler_work().  */
void sound_sampler_start();

/* Perform some sampling. */
void sample_noise();

/* Stop the sound sensor, report the current reading on the main event queue if pertinent.  */
void sound_sampler_stop();

#endif /* SNAPPY_READ_NOISE */

#endif /* !sound_sampler_h_included */
