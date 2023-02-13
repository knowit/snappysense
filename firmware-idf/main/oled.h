/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef oled_h_included
#define oled_h_included

#include "main.h"

#ifdef SNAPPY_OLED

bool oled_present();
void oled_clear_screen();
void oled_splash_screen();
void oled_show_text(const char* fmt, ...);

#endif

#endif /* !oled_h_included */
