// Interface to actual hardware device

#ifndef device_h_included
#define device_h_included

#include "main.h"
#include "sensor.h"
#include <time.h>

// Initialize the hardware.
void device_setup();

// Bring the signal on the periperal power line high, if it is currently low.  This will
// re-initialize the devices that were affected by power-off.  It is not possible to
// selectively initialize some devices.
void power_peripherals_on();

// Bring the signal on the periperal power line low UNCONDITIONALLY.  This has the effect
// of disabling the I2C devices (mic, gas, environment, and display in HW v1.0.0) as well
// as the PIR.  Note that the WAKE/BTN1 button and serial lines are not affected.
void power_peripherals_off();

// The following methods will have limited or no functionality if the peripherals have been
// powered off.  Some will report this fact on the log or on their output stream.
// Some will just do nothing.

// Show the splash screen on the device's builtin display.
void show_splash();

// Print the message on the device's builtin display.
void render_text(const char* value);

// Random bits, hopefully
unsigned long entropy();

// Print a reading on the display with the accompanying bitmap and unit description.
void render_oled_view(const uint8_t *bitmap, const char* value, const char *units);

// Read the sensors and store the readings in `*data`.
void get_sensor_values(SnappySenseData* data);

void reset_pir_and_mems();
void sample_pir();
void sample_mems();

// Go into a state where `msg` is displayed on all available surfaces and the
// device hangs.  If `is_error` is true then an additional error indication
// may be produced (eg a sound or additional message).
void enter_end_state(const char* msg, bool is_error = false) NO_RETURN;

#ifdef SNAPPY_PIEZO
void setup_sound();
void start_note(int frequency);
void stop_note();
#endif

#endif // !device_h_included
