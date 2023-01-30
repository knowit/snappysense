#ifndef snappy_i2c_h_included
#define snappy_i2c_h_included

#include <stdbool.h>
#include "driver/i2c.h"

/* Returns true if there is a device at the given address. */
bool i2cbus_probe(unsigned i2c_bus, unsigned address);

/* DOCUMENTME */
esp_err_t i2cbus_write_master(unsigned bus, unsigned address, const void* buffer, size_t nbytes,
			      unsigned timeoutMs);

/* DOCUMENTME */
esp_err_t i2cbus_read_master(unsigned i2c_bus, unsigned address, uint8_t* buff, size_t size,
			     unsigned timeOutMillis, size_t *readCount);

#endif /* !snappy_i2c_h_included */
