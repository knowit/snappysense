/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef piezo_h_included
#define piezo_h_included

#include "main.h"

#ifdef SNAPPY_GPIO_PIEZO
bool piezo_begin();
void piezo_start_note(int frequency);
void piezo_stop_note();
#endif

#endif /* !piezo_h_included */
