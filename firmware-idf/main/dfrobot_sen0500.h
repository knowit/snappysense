/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for environment sensor: DFRobot SEN0500
 * https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor
 */

#ifndef dfrobot_sen0500_h_included
#define dfrobot_sen0500_h_included

#include "main.h"

#ifdef SNAPPY_I2C_SEN0500

/* Device representation */
typedef struct {
  unsigned bus;			/* Zero-based i2c bus number */
  unsigned addr;		/* Unshifted i2c device address */
  unsigned timeout_ms;
} dfrobot_sen0500_t;

/* Initialize the device, filling in the fields of `self`. */
bool dfrobot_sen0500_begin(dfrobot_sen0500_t* self, unsigned i2c_bus, unsigned i2c_addr);

/* Temperature representation */
typedef enum {
  DFROBOT_SEN0500_TEMP_F,	/* Fahrenheit */
  DFROBOT_SEN0500_TEMP_C	/* Celsius */
} dfrobot_sen0500_temp_t;

/* Read the temperature register of the initialized device and return true on success, updating
 * *result; otherwise false.
 */
bool dfrobot_sen0500_get_temperature(dfrobot_sen0500_t* self, dfrobot_sen0500_temp_t tt,
				     float* result);

/* Read the humidity register of the initialized device and return true on success, updating
 * *result; otherwise false.  The unit of the output is relative humidity in percent; 50.0 is the
 * middle of the range.
 */
bool dfrobot_sen0500_get_humidity(dfrobot_sen0500_t* self, float* result);

/* Pressure representation */
typedef enum {
  DFROBOT_SEN0500_PRESSURE_KPA,	/* Kilopascal */
  DFROBOT_SEN0500_PRESSURE_HPA,	/* Hectopascal */
} dfrobot_sen0500_pressure_t;

/* Read the pressure register of the initialized device and return true on success, updating
 * *result; otherwise false.
 */
bool dfrobot_sen0500_get_atmospheric_pressure(dfrobot_sen0500_t* self, dfrobot_sen0500_pressure_t pt,
					      unsigned* result);

/* Read the uv intensity register of the initialized device and return true on success, updating
 * *result; otherwise false.  The output is in the range [0.0,15.0).
 *
 * TODO: The unit of the output is NOT DEFINED.  A standard UV index is an int in the range [0,11],
 * so it's not that.  The data sheet says the output is mW / m^2 [sic].  This is not a UV index
 * value, which is more complex, see Wikipedia.
 *
 * Most likely, rounding *result to the nearest integer is going to be OK.
 */
bool dfrobot_sen0500_get_ultraviolet_intensity(dfrobot_sen0500_t* self, float* result);

/* Read the light intensity register of the initialized device and return true on success, updating
 * *result; otherwise false.  The unit of the output is lux.
 */
bool dfrobot_sen0500_get_luminous_intensity(dfrobot_sen0500_t* self, float* result);

#endif /* SNAPPY_I2C_SEN0500 */

#endif /* !dfrobot_sen0500_h_included */
