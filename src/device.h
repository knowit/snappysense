// Interface to actual hardware device

#ifndef device_h_included
#define device_h_included

#include "main.h"
#include "sensor.h"

// Initialize the hardware.  If `*interactive_configuration` is set on return,
// then a request for config mode was detected during bootup.
void device_setup(bool* interactive_configuration);

// Show the splash screen on the device's builtin display.
void show_splash();

// Print the message on the device's builtin display.
void render_text(const char* value);

#ifdef DEMO_MODE
// Print a reading on the display with the accompanying bitmap and unit description.
void render_oled_view(const uint8_t *bitmap, const char* value, const char *units);
#endif

// Power off the display.
void power_off_display();

// Read the sensors and store the readings in `*data`.
void get_sensor_values(SnappySenseData* data);

// Go into a state where `msg` is displayed on all available surfaces and the
// device hangs.  If `is_error` is true then an additional error indication
// may be produced (eg a sound or additional message).
void enter_end_state(const char* msg, bool is_error);

// (Obscure) Bring the signal on the periperal power line low.
void power_off();

// (Obscure) Bring the signal on the periperal power line high.
void power_on();

// (Obscure) Probe the I2C bus for devices and report the findings in textual form
// on the output stream.
int probe_i2c_devices(Stream* output);

#ifdef TEST_MEMS
// (Obscure) Test the builtin MEMS
void test_mems();
#endif

#endif // !device_h_included
