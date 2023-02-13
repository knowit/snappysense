/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef esp32_i2c_h_included
#define esp32_i2c_h_included

#include "main.h"
#include "driver/i2c.h"

esp_err_t i2c_mem_write(unsigned i2c_num, unsigned device_address, unsigned mem_address,
                        uint8_t* write_buffer, size_t write_size) WARN_UNUSED;

#endif /* !esp32_i2c_h_included */
