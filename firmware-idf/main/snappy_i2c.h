#ifndef snappy_i2c_h_included
#define snappy_i2c_h_included

#include <stdbool.h>
#include <stdlib.h>

/* Returns true if there is a device at the given address. */
bool snappy_i2c_probe(unsigned i2c_bus, unsigned address);

/* Returns true if the write succeeded.  The address is unshifted. */
bool snappy_i2c_write(unsigned bus, unsigned address, const void* buffer, size_t nbytes,
		      unsigned timeoutMs);

/* Returns true if the read succeeded, and updates *readCount if so.
   The address is unshifted. */
bool snappy_i2c_read(unsigned i2c_bus, unsigned address, uint8_t* buff, size_t size,
		     unsigned timeOutMillis, size_t *readCount);

#endif /* !snappy_i2c_h_included */
