/* Manage the display - slideshow and error messages */

#ifndef slideshow_h_included
#define slideshow_h_included

#include "main.h"
#include "sensor.h"

// Advance the display, showing whatever's next
void slideshow_next();

// Upload new measurement data for the slideshow to use
void slideshow_new_data(SnappySenseData* new_data);

// Set a message to be displayed immediately, for the normal period, and then erased.
void slideshow_show_message_once(String* msg);

#endif // !slideshow_h_included
