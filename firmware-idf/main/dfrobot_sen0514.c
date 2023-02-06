/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/*
 * This code has been copied and/or adapted from the DFRobot_ENS160.{cpp,h} driver code for Arduino.
 *
 * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license  The MIT License (MIT)
 */

#include "dfrobot_sen0514.h"

#ifdef SNAPPY_I2C_SEN0514

#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

#define ENS160_PART_ID         0x160   ///< ENS160 chip version

/* Registers */
#define ENS160_PART_ID_REG     0x00   ///< 2 bytes, little-endian part number

#define ENS160_OPMODE_REG      0x10   ///< 1-byte, sets the Operating Mode of the ENS160.
#define ENS160_CONFIG_REG      0x11   ///< 1-byte, configures the action of the INTn pin.
#define ENS160_COMMAND_REG     0x12   ///< 1-byte, allows some additional commands to be executed on the ENS160.

#define ENS160_TEMP_IN_REG     0x13   ///< 2-bytes, allows host to write ambient temp data to ENS160 for compensation.
#define ENS160_RH_IN_REG       0x15   ///< 2-bytes, allows host to write relative humidity data to ENS160 for compensation.

#define ENS160_DATA_STATUS_REG 0x20   ///< This 1-byte register indicates the current STATUS of the ENS160.

#define ENS160_DATA_AQI_REG    0x21   ///< 1 byte, Air Quality Index according to the UBA.
#define ENS160_DATA_TVOC_REG   0x22   ///< 2 bytes, little-endian TVOC concentration in ppb.
#define ENS160_DATA_ECO2_REG   0x24   ///< 2 bytes, little-endian equivalent CO2-concentration in ppm,
                                      ///<   based on the detected VOCs and hydrogen.
#define ENS160_DATA_ETOH_REG   0x22   ///< 2 bytes, calculated ethanol concentration in ppb.

/* OPMODE(Address 0x10) register mode */
#define ENS160_SLEEP_MODE      0x00   ///< DEEP SLEEP mode (low power standby).
#define ENS160_IDLE_MODE       0x01   ///< IDLE mode (low-power).
#define ENS160_STANDARD_MODE   0x02   ///< STANDARD Gas Sensing Modes.

/*  The status of interrupt pin when new data appear in DATA_XXX */
#define INT_DATA_DRDY_DIS (0 << 1) /* Disable */
#define INT_DATA_DRDY_EN (1 << 1)  /* Enable */

/* The status of interrupt pin when new data appear in General Purpose Read Registers */
#define INT_GPR_DRDY_DIS (0 << 3) /* Disable */
#define INT_GPR_DRDY_EN (1 << 3)  /* Enable */

/* Interrupt pin main switch mode */
#define INT_MODE_DIS 0		/* Disable */
#define INT_MODE_EN 1		/* Enable */

static bool write_reg8(dfrobot_sen0514_t* self, unsigned reg, unsigned val) {
  uint8_t msg[2] = { reg, val };
  return i2c_master_write_to_device(self->bus, self->address, msg, sizeof(msg),
				    pdMS_TO_TICKS(self->timeout_ms)) == ESP_OK;
}

static bool read_reg16(dfrobot_sen0514_t* self, unsigned reg, unsigned* response) {
  uint8_t msg = reg;
  uint8_t buf[2];
  if (i2c_master_write_read_device(self->bus, self->address, &msg, sizeof(msg), buf, sizeof(buf),
				   pdMS_TO_TICKS(self->timeout_ms)) != ESP_OK) {
    return false;
  }
  *response = (buf[1] << 8) | buf[0];
  return true;
}

static bool read_reg8(dfrobot_sen0514_t* self, unsigned reg, unsigned* response) {
  uint8_t msg = reg;
  uint8_t buf;
  if (i2c_master_write_read_device(self->bus, self->address, &msg, sizeof(msg), &buf, sizeof(buf),
				   pdMS_TO_TICKS(self->timeout_ms)) != ESP_OK) {
    return false;
  }
  *response = buf;
  return true;
}

static bool set_power_mode(dfrobot_sen0514_t* self, unsigned mode) {
  if (!write_reg8(self, ENS160_OPMODE_REG, mode)) {
    LOG("SEN0514: Failed to set power mode\n");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(20)); 
  return true;
}

static bool set_interrupt_mode(dfrobot_sen0514_t* self, unsigned mode) {
  if (!write_reg8(self, ENS160_CONFIG_REG, mode | INT_DATA_DRDY_EN | INT_GPR_DRDY_DIS)) {
    LOG("SEN0514: Failed to set interrupt mode\n");
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(20)); 
  return true;
}

bool dfrobot_sen0514_begin(dfrobot_sen0514_t* self, unsigned i2c_bus, unsigned i2c_addr) {
  self->bus = i2c_bus;
  self->address = i2c_addr;
  self->timeout_ms = 200;
  unsigned response;
  if (!read_reg16(self, ENS160_PART_ID_REG, &response)) {
    LOG("SEN0514: Could not read part ID\n");
    return false;
  }
  if (ENS160_PART_ID != response) {
    LOG("SEN0514: Unexpected part number %u, wanted %u\n", response, ENS160_PART_ID);
    return false;
  }
  set_power_mode(self, ENS160_STANDARD_MODE);
  set_interrupt_mode(self, INT_MODE_DIS);
  return true;
}

bool dfrobot_sen0514_get_sensor_status(dfrobot_sen0514_t* self, dfrobot_sen0514_status_t* result) {
  unsigned response;
  if (!read_reg8(self, ENS160_DATA_STATUS_REG, &response)) {
    return false;
  }
  *result = (dfrobot_sen0514_status_t)((response >> 2) & 3);
  return true;
}

bool dfrobot_sen0514_prime(dfrobot_sen0514_t* self, float temperature, float humidity) {
  unsigned t = (unsigned)((temperature + 273.15f) * 64.0f);
  unsigned rh = (unsigned)(humidity * 512.0f);
  uint8_t buf[5];

  buf[0] = ENS160_TEMP_IN_REG;
  buf[1] = t & 0xFF;
  t >>= 8;
  buf[2] = t & 0xFF;
  buf[3] = rh & 0xFF;
  rh >>= 8;
  buf[4] = rh & 0xFF;
  return i2c_master_write_to_device(self->bus, self->address, buf, sizeof(buf),
				    pdMS_TO_TICKS(self->timeout_ms)) == ESP_OK;
  /* FIXME: Should we wait 20ms here? */
}

bool dfrobot_sen0514_get_air_quality_index(dfrobot_sen0514_t* self, unsigned* result) {
  return read_reg8(self, ENS160_DATA_AQI_REG, result);
}

bool dfrobot_sen0514_get_total_volatile_organic_compounds(dfrobot_sen0514_t* self, unsigned* result) {
  return read_reg16(self, ENS160_DATA_TVOC_REG, result);
}

bool dfrobot_sen0514_get_co2(dfrobot_sen0514_t* self, unsigned* result) {
  return read_reg16(self, ENS160_DATA_ECO2_REG, result);
}

#endif
