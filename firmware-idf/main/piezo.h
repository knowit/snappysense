/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef piezo_h_included
#define piezo_h_included

#include "main.h"

struct tone {
  uint16_t frequency;
  uint16_t duration_ms;
};

struct music {
  unsigned length;
  const struct tone* tones;
};

/* Initialize the music subsystem.  Call from main task during system initialization.  This will
   create a music player task. */
bool piezo_begin() WARN_UNUSED;

/* Start the song.  This can be called from any task.  It will stop whatever song was playing, if
   any. */
void piezo_play(const struct music* song);

/* Stop the song playing, if any.  This can be called from any task. */
void piezo_stop();

#endif // !piezo_h_included
