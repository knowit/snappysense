#ifndef dfrobot_sen0500_h_included
#define dfrobot_sen0500_h_included

#include <stdbool.h>
#include <stddef.h>

/* DFRobot SEN0500 environmental sensor device, on i2c */

typedef struct {
  unsigned timeout_ms;
  unsigned bus;
  unsigned addr;		/* Unshifted address */
} dfrobot_sen0500_t;

typedef enum {
  DFROBOT_SEN0500_TEMP_F,
  DFROBOT_SEN0500_TEMP_C
} dfrobot_sen0500_temp_t;

/* Initialize the device, filling in the fields of `self`. */
bool dfrobot_sen0500_begin(dfrobot_sen0500_t* self, unsigned i2c_bus, unsigned i2c_addr);

/* Read the temperature register of the initialized device and return
   true on success, updating *result; otherwise false.
 */
bool dfrobot_sen0500_get_temperature(dfrobot_sen0500_t* self, dfrobot_sen0500_temp_t tt, float* result);

#endif /* !dfrobot_sen0500_h_included */
