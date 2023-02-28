// Implement button logic from raw down/up events.

#ifndef button_h_included
#define button_h_included

#include "main.h"

// Initialize the button subsystem.  Call this early.
void button_init();

// Call this when the button is pressed.
void button_down();

// Call this when the button is released.
void button_up();

#endif // !button_h_included
