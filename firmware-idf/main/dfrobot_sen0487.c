/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "dfrobot_sen0487.h"

#ifdef SNAPPY_ADC_SEN0487

/* FIXME: Include something from the ADC subsystem */
/* This looks like an ADC oneshot mode situation */
#include "esp_adc/adc_oneshot.h"
/* Whether calibration is needed is TBD.  See app example */

unsigned sen0487_sound_level() {
  /* FIXME: analogRead is Arduino */
  //  unsigned r = analogRead(MIC_PIN);
  unsigned r = 0;
  /* TODO - This scale is pretty arbitrary */
  if (r < 1700) {
    return 1;
  }
  if (r < 2000) {
    return 2;
  }
  if (r < 2500) {
    return 3;
  }
  if (r < 3000) {
    return 4;
  }
  return 5;
}

#endif
