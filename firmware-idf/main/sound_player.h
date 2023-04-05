/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef sound_player_h_included
#define sound_player_h_included

#include "main.h"

#ifdef SNAPPY_SOUND_EFFECTS

# error "Not integrated with the new main loop"

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

/* Do some work. */
void sound_effects_work();

/* Stop the song playing, if any.  This can be called from any task. */
void sound_effects_stop();

#endif	/* SNAPPY_SOUND_EFFECTS */

#endif /* !sound_player_h_included */
