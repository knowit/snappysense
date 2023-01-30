/*
// The following excerpted and rewritten from the Arduino DFRobot_EnvironmentalSensor.{cpp,h} files
//
// @copyright	Copyright (c) 2021 DFRobot Co.Ltd (http://www.dfrobot.com)
// @license   The MIT License (MIT)
*/

#include "dfrobot_sen0500.h"

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

static void read_register(dfrobot_sen0500_t* self, unsigned reg, void *buffer, size_t nbytes) {
  /* A register read command writes the register number shifted left and then reads data */
  uint8_t operation = reg << 1;
  i2cbus_write_master(self->bus, self->addr, &operation, 1, self->timeout_ms);
  /* TODO: Check that the number of bytes read is correct */
  size_t bytes_read;
  i2cbus_read_master(self->bus, self->addr, buffer, nbytes, self->timeout_ms, &bytes_read);
}

static uint16_t read_register_u16(dfrobot_sen0500_t* self, unsigned reg) {
  uint8_t buf[2];
  read_register(self, reg, buf, 2);
  return (buf[0] << 8) | buf[1];
}
  
bool dfrobot_sen0500_begin(dfrobot_sen0500_t* self, unsigned i2c_bus, unsigned i2c_addr) {
  if (i2c_addr < 1 || i2c_addr > 0x7F) {
    return false;		/* Invalid i2c address */
  }
  self->timeout_ms = 200;
  self->bus = i2c_bus;
  self->addr = i2c_addr;
  unsigned response = read_register_u16(self, REG_DEVICE_ADDR) & 0xFF;
  if ((self->addr << 1) != response) {
    return false;		/* Invalid i2c address or device response */
  }
  return true;
}


float dfrobot_sen0500_get_temperature(dfrobot_sen0500_t* self, temp_type_t tt) {
  int16_t data = (int16_t)read_register_u16(self, REG_TEMP);
  float temp = (-45) + (data * 175.00) / 1024.00 / 64.00;
  if(tt == TEMP_F) {
    temp = temp * 1.8 + 32 ;
  }
  return temp;
}
