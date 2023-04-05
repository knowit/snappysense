/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef slideshow_h_included
#define slideshow_h_included

#include "main.h"
#include "sensor.h"

/* Initialize but don't show anything yet, returns false on failure */
bool slideshow_begin() WARN_UNUSED;

/* Start showing things */
void slideshow_start();

/* Stop showing things */
void slideshow_stop();

/* Advance to the next thing, according to internal logic */
void slideshow_next();

/* Reset slideshow to start of cycle */
void slideshow_reset();

/* Takes ownership of data, which is always malloc'd */
void slideshow_new_data(sensor_state_t* data);

/* Takes ownership of msg, which is always malloc'd */
void slideshow_show_message_once(char* msg);

#endif /* !slideshow_h_included */
