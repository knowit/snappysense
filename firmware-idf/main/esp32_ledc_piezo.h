/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef esp32_ledc_piezo_h_included
#define esp32_ledc_piezo_h_included

#include "main.h"

#ifdef SNAPPY_ESP32_LEDC_PIEZO
bool piezo_begin();
void piezo_start_note(int frequency);
void piezo_stop_note();
#endif

#endif /* !esp32_ledc_piezo_h_included */
