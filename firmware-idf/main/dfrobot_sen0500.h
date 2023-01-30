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
  TEMP_F,
  TEMP_C
} temp_type_t;

/* Initialize the device, filling in the fields of `self`. */
bool dfrobot_sen0500_begin(dfrobot_sen0500_t* self, unsigned i2c_bus, unsigned i2c_addr);

/* Read the temperature register of the initialized device */
float dfrobot_sen0500_get_temperature(dfrobot_sen0500_t* self, temp_type_t tt);

#endif /* !dfrobot_sen0500_h_included */
