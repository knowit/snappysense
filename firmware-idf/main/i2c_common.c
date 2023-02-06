#include "i2c_common.h"

#ifdef SNAPPY_I2C

#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

static bool read_register(i2c_common_t* self, unsigned reg, void *buffer, size_t nbytes) {
  /* TODO: The correct thing to do here is to use the i2c_master_write_read_device operation,
     to avoid relinquishing the bus.  This prevents any mishaps with tasks, should we end
     up having background tasks that access the SEN0500 as well as the main task.  For example,
     the synchronous `read` command (serial console) along with a background task that gathers
     sensor readings would create that situation. */
  /* A register read command writes the register number shifted left and then reads data */
  uint8_t operation = reg << 1;
  esp_err_t res;
  res = i2c_master_write_to_device(self->bus, self->address, &operation, 1,
				   self->timeout_ms / portTICK_PERIOD_MS);
  if (res != ESP_OK) {
    LOG("Byte write to %d @ %02X failed %d with timeout %d\n", self->bus, self->address, res,
	self->timeout_ms);
    return false;
  }
  res = i2c_master_read_from_device(self->bus, self->address, buffer, nbytes, self->timeout_ms);
  if (res != ESP_OK) {
    LOG("Failed to read response\n");
    return false;
  }
  return true;
}

bool read_i2c_reg16(i2c_common_t* self, unsigned reg, unsigned* result) {
  uint8_t buf[2];
  if (!read_register(self, reg, buf, 2)) {
    return false;
  }
  *result = ((unsigned)buf[0] << 8) | buf[1];
  return true;
}
  
#endif
