/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef piezo_h_included
#define piezo_h_included

#include "main.h"

#ifdef SNAPPY_SOUND_EFFECTS

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
bool sound_effects_begin() WARN_UNUSED;

/* Start the song.  This can be called from any task.  It will stop whatever song was playing, if
   any. */
void sound_effects_play(const struct music* song);

/* Stop the song playing, if any.  This can be called from any task. */
void sound_effects_stop();

#endif	/* SNAPPY_SOUND_EFFECTS */

#endif /* !piezo_h_included */
