/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for air/gas sensor: DFRobot SEN0514.
 * https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor
 */

#ifndef dfrobot_sen0514_h_included
#define dfrobot_sen0514_h_included

#include "main.h"

#ifdef SNAPPY_I2C_SEN0514

/* Device representation */
typedef struct {
  unsigned bus;			/* Zero-based */
  unsigned address;		/* Unshifted bus address */
  unsigned timeout_ms;
} dfrobot_sen0514_t;

/* Initialize the device, filling in the fields of `self`. */
bool dfrobot_sen0514_begin(dfrobot_sen0514_t* self, unsigned i2c_bus, unsigned i2c_addr) WARN_UNUSED;

/* Make of these what you will, but value 0 at least means we're OK */
typedef enum {
  DFROBOT_SEN0514_NORMAL_OPERATION = 0,
  DFROBOT_SEN0514_WARMUP_PHASE = 1,
  DFROBOT_SEN0514_INITIAL_STARTUP_PHASE = 2,
  DFROBOT_SEN0514_INVALID_OUTPUT = 3
} dfrobot_sen0514_status_t;

bool dfrobot_sen0514_get_sensor_status(dfrobot_sen0514_t* self, dfrobot_sen0514_status_t* result) WARN_UNUSED;

/* Prime the device with temperature and humidity, to ensure readings are sensible.
 * Temperature is degrees celsius, [-273, whatever)
 * Humidity is relative humidity, [0,1]
 *
 * TODO: How often can/must we do this?
 */
bool dfrobot_sen0514_prime(dfrobot_sen0514_t* self, float temperature, float humidity) WARN_UNUSED;

/* Range 1-5: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy */
bool dfrobot_sen0514_get_air_quality_index(dfrobot_sen0514_t* self, unsigned* result) WARN_UNUSED;

/* Range 0–65000: Parts per billion */
bool dfrobot_sen0514_get_total_volatile_organic_compounds(dfrobot_sen0514_t* self, unsigned* result) WARN_UNUSED;

/* Range 400–65000: Parts per million. */
bool dfrobot_sen0514_get_co2(dfrobot_sen0514_t* self, unsigned* result) WARN_UNUSED;

#endif

#endif /* !dfrobot_sen0514_h_included */
