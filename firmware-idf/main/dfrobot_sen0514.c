/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "dfrobot_sen0514.h"

#ifdef SNAPPY_I2C_SEN0514

#define ENS160_DATA_AQI_REG    uint8_t(0x21)   ///< This 1-byte register reports the calculated Air Quality Index according to the UBA.
#define ENS160_DATA_TVOC_REG   uint8_t(0x22)   ///< This 2-byte register reports the calculated TVOC concentration in ppb.
#define ENS160_DATA_ECO2_REG   uint8_t(0x24)   ///< This 2-byte register reports the calculated equivalent CO2-concentration in ppm, based on the detected VOCs and hydrogen.
#define ENS160_DATA_ETOH_REG   uint8_t(0x22)   ///< This 2-byte register reports the calculated ethanol concentration in ppb.


bool dfrobot_sen0514_begin(dfrobot_sen0514_t* self, unsigned i2c_bus, unsigned i2c_addr) {
}

bool dfrobot_sen0514_prime(dfrobot_sen0514_t* self, float temperature, float humidity) {
}

bool dfrobot_sen0514_get_air_quality_index(dfrobot_sen0514_t* self, unsigned* result) {
  return read_i2c_reg8(self, ENS160_DATA_AQI_REG, &result);
}

bool dfrobot_sen0514_get_total_volatile_organic_compounds(dfrobot_sen0514_t* self, unsigned* result) {
  return read_i2c_reg16(self, ENS160_DATA_TVOC_REG, &result);
}

bool dfrobot_sen0514_get_co2(dfrobot_sen0514_t* self, unsigned* result) {
  return read_i2c_reg16(self, ENS160_DATA_ECO2_REG, &result);
}

#endif
