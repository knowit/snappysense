/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef esp32_i2c_h_included
#define esp32_i2c_h_included

#include "main.h"
#include "driver/i2c.h"

/* The i2c layer exports the following types and functions, used by the drivers.  This will be
   generalized a little bit (away from ESP32) when we port to other platforms.

typedef enum {
  ESP_OK = 0,
  ...
} esp_err_t;

esp_err_t i2c_mem_write(unsigned i2c_num, unsigned device_address, unsigned mem_address,
                        const uint8_t* write_buffer, size_t write_size);

esp_err_t i2c_master_write_read_device(unsigned i2c_num, unsigned device_address,
                                       const uint8_t* write_buffer, size_t write_size,
                                       uint8_t* read_buffer, size_t read_size,
                                       unsigned timeout_ticks);

esp_err_t i2c_master_write_to_device(unsigned i2c_num, unsigned device_address,
                                       const uint8_t* write_buffer, size_t write_size,
                                       unsigned timeout_ticks);

 */

esp_err_t i2c_mem_write(unsigned i2c_num, unsigned device_address, unsigned mem_address,
                        const uint8_t* write_buffer, size_t write_size) WARN_UNUSED;

#endif /* !esp32_i2c_h_included */
