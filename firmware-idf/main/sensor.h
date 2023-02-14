/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef sensor_h_included
#define sensor_h_included

#include "main.h"

/* Sensor readings. Zero-initializing this brings it to a known good state. */
typedef struct {
  /* Degrees C */
  bool have_temperature;
  float temperature;

  /* Percent relative humidity */
  bool have_humidity;
  float humidity;

  /* 10^2 Pascal */
  bool have_atmospheric_pressure;
  unsigned atmospheric_pressure;

  /* [0,15), supposedly a UV index value */
  bool have_uv_intensity;
  float uv_intensity;

  /* Lux */
  bool have_luminous_intensity;
  float luminous_intensity;

  /* Equivalent CO_2 content, ppm */
  bool have_co2;
  unsigned co2;

  /* Total volatile organic compounds, ppb */
  bool have_tvoc;
  unsigned tvoc;

  /* 1..5, air quality index */
  bool have_aqi;
  unsigned aqi;

  /* Whether motion was detected during the monitoring window or not, true or false.  It doesn't
     look like we have any way of detecting whether there is a motion sensor or not. */
  bool motion;

  /* Sound level on a scale from 1 (quiet) to 5 (unbearable) */
  bool have_sound_level;
  unsigned sound_level;
} sensor_state_t;

bool sensor_begin() WARN_UNUSED;   /* True on success, false on failure */
void open_monitoring_window();     /* In response to EV_SENSOR_CLOCK */
void close_monitoring_window();    /* In response to EV_MONITORING_CLOCK */
void record_motion();              /* In response to EV_MOTION */
void record_noise(uint32_t level); /* In response to EV_SOUND_SAMPLE */

/* Global object for current readings */
extern sensor_state_t sensor;

/* Abstraction leak: information about whether this specific sensor has been calibrated, used for
   some logging.

   TODO: Maybe this belongs in the device layer somehow? */
#ifdef SNAPPY_I2C_SEN0514
extern bool have_calibrated_sen0514;
#endif

#endif /* !sensor_h_included */
