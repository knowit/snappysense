/*
// The following excerpted and rewritten from the Arduino DFRobot_EnvironmentalSensor.{cpp,h} files
//
// @copyright	Copyright (c) 2021 DFRobot Co.Ltd (http://www.dfrobot.com)
// @license   The MIT License (MIT)
*/

#include "dfrobot_sen0500.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

#include <stdint.h>
#include "snappy_i2c.h"

#define REG_PID                   0x0000 ///< Register for protocol transition adapter
#define REG_VID                   0x0001 ///< Register for protocol transition adapter
#define REG_DEVICE_ADDR           0x0002 ///< Register for protocol transition adapter
#define REG_UART_CTRL0            0x0003 ///< Register for protocol transition adapter
#define REG_UART_CTRL1            0x0004 ///< Register for protocol transition adapter
#define REG_VERSION               0x0005 ///< Register for protocol transition adapter

#define REG_ULTRAVIOLET_INTENSITY 0x0008 ///< Register for protocol transition adapter
#define REG_LUMINOUS_INTENSITY    0x0009 ///< Register for protocol transition adapter
#define REG_TEMP                  0x000A ///< Register for protocol transition adapter
#define REG_HUMIDITY              0x000B ///< Register for protocol transition adapter
#define REG_ATMOSPHERIC_PRESSURE  0x000C ///< Register for protocol transition adapter
//#define REG_ELEVATION             0x000D ///< Register for protocol transition adapter

static bool read_register(dfrobot_sen0500_t* self, unsigned reg, void *buffer, size_t nbytes) {
  /* TODO: The correct thing to do here is to use a write_then_read operation if possible,
     to avoid relinquishing the bus.  This prevents any mishaps with tasks, should we end
     up having background tasks that access the SEN0500 as well as the main task.  For example,
     the synchronous `read` command (serial console) along with a background task that gathers
     sensor readings would create that situation. */
  /* A register read command writes the register number shifted left and then reads data */
  uint8_t operation = reg << 1;
  esp_err_t res;
  res = i2c_master_write_to_device(self->bus, self->addr, &operation, 1, self->timeout_ms / portTICK_PERIOD_MS);
  if (res != ESP_OK) {
    LOG("Byte write to %d @ %02X failed %d with timeout %d\n", self->bus, self->addr, res, self->timeout_ms);
    return false;
  }
  res = i2c_master_read_from_device(self->bus, self->addr, buffer, nbytes, self->timeout_ms);
  if (res != ESP_OK) {
    LOG("Failed to read response\n");
    return false;
  }
  return true;
}

static bool read_register_u16(dfrobot_sen0500_t* self, unsigned reg, unsigned* result) {
  uint8_t buf[2];
  if (!read_register(self, reg, buf, 2)) {
    return false;
  }
  *result = ((unsigned)buf[0] << 8) | buf[1];
  return true;
}
  
bool dfrobot_sen0500_begin(dfrobot_sen0500_t* self, unsigned i2c_bus, unsigned i2c_addr) {
  if (i2c_addr < 1 || i2c_addr > 0x7F) {
    LOG("Bad address\n");
    return false;		/* Invalid i2c address */
  }
  self->timeout_ms = 200;
  self->bus = i2c_bus;
  self->addr = i2c_addr;
  unsigned response = 0;
  if (!read_register_u16(self, REG_DEVICE_ADDR, &response)) {
    LOG("Read failed\n");
    return false;
  }
  if (self->addr != (response & 0xFF)) {
    LOG("Bad response %04X addr %04X\n", response, self->addr);
    return false;		/* Invalid i2c address or device response */
  }
  return true;
}


bool dfrobot_sen0500_get_temperature(dfrobot_sen0500_t* self, dfrobot_sen0500_temp_t tt, float* result) {
  unsigned response = 0;
  if (!read_register_u16(self, REG_TEMP, &response)) {
    return false;
  }
  float reading = (float)(int16_t)response;
  reading = -45.0f + (reading * 175.00f) / 1024.0f / 64.0f;
  if(tt == DFROBOT_SEN0500_TEMP_F) {
    reading = reading * 1.8f + 32.0f;
  }
  *result = reading;
  return true;
}
