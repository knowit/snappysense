/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "esp32_i2c.h"

esp_err_t i2c_mem_write(unsigned i2c_num, unsigned device_address, unsigned mem_address,
                   uint8_t* write_buffer, size_t write_size) {
  /* Based on i2c_master_write_to_device() */

  /* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
     SPDX-License-Identifier: Apache-2.0 */

  esp_err_t err = ESP_OK;
  uint8_t buffer[I2C_LINK_RECOMMENDED_SIZE(2)] = { 0 };

  i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
  assert (handle != NULL);

  err = i2c_master_start(handle);
  if (err != ESP_OK) {
    goto end;
  }

  err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_WRITE, true);
  if (err != ESP_OK) {
    goto end;
  }

  err = i2c_master_write_byte(handle, mem_address, true);
  if (err != ESP_OK) {
    goto end;
  }
  
  err = i2c_master_write(handle, write_buffer, write_size, true);
  if (err != ESP_OK) {
    goto end;
  }

  i2c_master_stop(handle);
  err = i2c_master_cmd_begin(i2c_num, handle, portMAX_DELAY);

 end:
  i2c_cmd_link_delete_static(handle);
  return err;
}
