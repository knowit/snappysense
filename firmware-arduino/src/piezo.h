// Simple music player.  It plays a song on the piezo-element speaker until the song ends,
// it is told to stop, or it is told to play a different song.  The player runs on a separate
// FreeRTOS task, which sleeps if there's nothing to do.

#ifndef piezo_h_included
#define piezo_h_included

#include "main.h"

#ifdef SNAPPY_PIEZO
// Play a song.  The syntax for the song is defined in a comment in the cpp file
void play_song(const char* song);

// Stop playing a song, if it's playing.
void stop_song();
#endif

#endif // !piezo_h_included
