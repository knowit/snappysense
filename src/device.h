// Interface to actual hardware device

#ifndef device_h_included
#define device_h_included

#include "main.h"

void device_setup();
void power_on();
void power_off();
int probe_i2c_devices(Stream* stream);
void get_sensor_values();
void show_splash();
#ifdef STANDALONE
void render_oled_view(const uint8_t *bitmap, const char* value, const char *units);
#endif

#endif // !device_h_included
