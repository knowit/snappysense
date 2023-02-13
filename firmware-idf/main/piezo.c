/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "piezo.h"

/* FIXME: This is arduino stuff.  Instead, we want to include "driver/ledc.h" and we want to control
   the pulse in the same way that the arduino library does.  Some inspiration for this may be taken
   from the way Gunnar did it in his prototype. */

//#include "esp32-hal-ledc.h"

bool piezo_begin() {
  // TODO: Justify these choices, which are wrong.  Probably 1 bit of resolution is fine?
  // And the question is whether ledcSetup should be before or after ledcAttachPin.
  //ledcSetup(PWM_CHAN, 4000, 16);
  //  ledcAttachPin(PWM_PIN, PWM_CHAN);
  return true;
}

void piezo_start_note(int frequency) {
  //  ledcWriteTone(PWM_CHAN, frequency);
}

void piezo_stop_note() {
  //  ledcWrite(PWM_CHAN, 0);
}
