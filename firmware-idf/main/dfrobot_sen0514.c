#include "dfrobot_sen0514.h"

#ifdef SNAPPY_I2C_SEN0514

bool dfrobot_sen0514_begin(dfrobot_sen0514_t* self, unsigned i2c_bus, unsigned i2c_addr) {
}

bool dfrobot_sen0514_prime(dfrobot_sen0514_t* self, float temperature, float humidity) {
}

bool dfrobot_sen0514_get_air_quality_index(dfrobot_sen0514_t* self, unsigned* result) {
}

bool dfrobot_sen0514_get_total_volatile_organic_compounds(dfrobot_sen0514_t* self, unsigned* result) {
}

bool dfrobot_sen0514_get_co2(dfrobot_sen0514_t* self, unsigned* result) {
}

#endif
