/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef dfrobot_sen0487_h_included
#define dfrobot_sen0487_h_included

#include "main.h"

#ifdef SNAPPY_GPIO_SEN0487
/* Scale of 1 to 5, 0 means error of some sort. */
unsigned sen0487_sound_level();
#endif

#endif /* !dfrobot_sen0487_h_included */
