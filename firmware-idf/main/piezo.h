/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

// Simple music player.  It plays a song on the piezo-element speaker until the song ends,
// it is told to stop, or it is told to play a different song.  The player runs on a separate
// FreeRTOS task, which sleeps if there's nothing to do.

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

void play_song(const struct music* song);
void stop_song();

#endif // !piezo_h_included
