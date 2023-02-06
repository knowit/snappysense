/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef i2c_common_h_included
#define i2c_common_h_included

#include "main.h"

#ifdef SNAPPY_I2C

typedef struct {
  unsigned bus;			/* Zero-based */
  unsigned address;		/* Unshifted bus address */
  unsigned timeout_ms;
} i2c_common_t;

bool read_i2c_reg16(i2c_common_t* self, unsigned reg, unsigned* result);

#endif

#endif /* !i2c_common_h_included */
