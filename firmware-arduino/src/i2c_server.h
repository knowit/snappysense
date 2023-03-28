// A server that reads from the I2C slave interface.

#ifndef i2c_server_h_included
#define i2c_server_h_included

#include "main.h"

#ifdef SNAPPY_I2C_SERVER

void i2c_server_init();
void i2c_server_start();
void i2c_server_poll();
void i2c_server_stop();

#endif // SNAPPY_I2C_SERVER

#endif // !i2c_server_h_included
