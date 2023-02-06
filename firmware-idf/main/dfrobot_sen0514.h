/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for air/gas sensor: DFRobot SEN0514.
 * https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor
 */

#ifndef dfrobot_sen0514_h_included
#define dfrobot_sen0514_h_included

#include "main.h"

#ifdef SNAPPY_I2C_SEN0514

#include "i2c_common.h"

/* Device representation */
typedef i2c_common_t dfrobot_sen0514_t;

/* Initialize the device, filling in the fields of `self`. */
bool dfrobot_sen0514_begin(dfrobot_sen0514_t* self, unsigned i2c_bus, unsigned i2c_addr);

/* Prime the device with temperature and humidity, to ensure readings are sensible.
 * Temperature is degrees celsius, [-273, whatever)
 * Humidity is relative humidity, [0,1]
 *
 * TODO: How often can/must we do this?
 */
bool dfrobot_sen0514_prime(dfrobot_sen0514_t* self, float temperature, float humidity);

/* Range 1-5: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy */
bool dfrobot_sen0514_get_air_quality_index(dfrobot_sen0514_t* self, unsigned* result);

/* Range 0–65000: Parts per billion */
bool dfrobot_sen0514_get_total_volatile_organic_compounds(dfrobot_sen0514_t* self, unsigned* result);

/* Range 400–65000: Parts per million. */
bool dfrobot_sen0514_get_co2(dfrobot_sen0514_t* self, unsigned* result);

#endif

#endif /* !dfrobot_sen0514_h_included */
