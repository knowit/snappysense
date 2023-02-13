/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for environment sensor: DFRobot SEN0500
 
   Much of the following is taken from the Arduino DFRobot_EnvironmentalSensor.{cpp,h} files,
   covered by the following copyright:
 
   @copyright Copyright (c) 2021 DFRobot Co.Ltd (http://www.dfrobot.com)
   @license   The MIT License (MIT) */

#include "dfrobot_sen0500.h"

#include "driver/i2c.h"

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

static bool read_reg16(dfrobot_sen0500_t* self, unsigned reg, unsigned* response) {
  uint8_t msg = reg << 1;
  uint8_t buf[2];
  if (i2c_master_write_read_device(self->bus, self->address, &msg, sizeof(msg), buf, sizeof(buf),
				   pdMS_TO_TICKS(self->timeout_ms)) != ESP_OK) {
    return false;
  }
  *response = ((unsigned)buf[0] << 8) | buf[1];
  return true;
}

bool dfrobot_sen0500_begin(dfrobot_sen0500_t* self, unsigned i2c_bus, unsigned i2c_addr) {
  self->bus = i2c_bus;
  self->address = i2c_addr;
  self->timeout_ms = 200;
  unsigned response = 0;
  if (!read_reg16(self, REG_DEVICE_ADDR, &response)) {
    LOG("SEN0500: Device init read failed");
    return false;
  }
  if (self->address != (response & 0xFF)) {
    LOG("SEN0500: Device init bad response %04X want %04X", response, self->address);
    return false;		/* Invalid i2c address or device response */
  }
  return true;
}

bool dfrobot_sen0500_get_temperature(dfrobot_sen0500_t* self, dfrobot_sen0500_temp_t tt,
				     float* result) {
  unsigned response = 0;
  if (!read_reg16(self, REG_TEMP, &response)) {
    LOG("SEN0500: Temperature read failed");
    return false;
  }
  float reading = (float)(int16_t)response;
  reading = -45.0f + (reading * 175.0f) / 1024.0f / 64.0f;
  if(tt == DFROBOT_SEN0500_TEMP_F) {
    reading = reading * 1.8f + 32.0f;
  }
  *result = reading;
  return true;
}

bool dfrobot_sen0500_get_humidity(dfrobot_sen0500_t* self, float* result) {
  unsigned response = 0;
  if (!read_reg16(self, REG_HUMIDITY, &response)) {
    LOG("SEN0500: Humidity read failed");
    return false;
  }
  *result = (float)response * 100.0f / 65536.0f;
  return true;
}

bool dfrobot_sen0500_get_atmospheric_pressure(dfrobot_sen0500_t* self, dfrobot_sen0500_pressure_t pt,
					      unsigned* result) {
  unsigned response;
  if (!read_reg16(self, REG_ATMOSPHERIC_PRESSURE, &response)) {
    LOG("SEN0500: Pressure read failed");
    return false;
  }
  if (pt == DFROBOT_SEN0500_PRESSURE_KPA) {
    response /= 10;
  }
  *result = response;
  return true;
}

/* Map x that is within the interval [in_min,in_max) to its appropriate point in the interval
   [out_min,out_max).  */
static float map_float(float x, float in_min, float in_max, float out_min, float out_max) {
  return out_min + ((x - in_min) * (out_max - out_min) / (in_max - in_min));
}

bool dfrobot_sen0500_get_ultraviolet_intensity(dfrobot_sen0500_t* self, float* result) {
  unsigned response;
  if (!read_reg16(self, REG_ULTRAVIOLET_INTENSITY, &response)) {
    LOG("SEN0500: UV read failed");
    return false;
  }
  float outputVoltage = (3.0f * (float)response) / 1024.0f;
  *result = map_float(outputVoltage, 0.99f, 2.9f, 0.0f, 15.0f);
  return true;
}

bool dfrobot_sen0500_get_luminous_intensity(dfrobot_sen0500_t* self, float* result) {
  unsigned response;
  if (!read_reg16(self, REG_LUMINOUS_INTENSITY, &response)) {
    LOG("SEN0500: Lux read failed");
    return false;
  }
  float luminous = (float)response;
  *result = luminous * (1.0023f +
			luminous * (8.1488e-5f +
				    luminous * (-9.3924e-9f +
						luminous * 6.0135e-13f)));
  return true;
}
