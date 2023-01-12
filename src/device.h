// Interface to actual hardware device

#ifndef device_h_included
#define device_h_included

#include "main.h"
#include "sensor.h"

void device_setup(bool* interactive_configuration);
void power_on();
void power_off();
int probe_i2c_devices(Stream* stream);
void get_sensor_values(SnappySenseData* data);
void show_splash();
void render_text(const char* value);
#ifdef DEMO_MODE
void render_oled_view(const uint8_t *bitmap, const char* value, const char *units);
#endif
#ifdef TEST_MEMS
void test_mems();
#endif

#endif // !device_h_included
