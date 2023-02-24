/* Manage the display - slideshow and error messages */

#ifndef slideshow_h_included
#define slideshow_h_included

#include "main.h"
#include "sensor.h"

// Set up the slideshow but don't show anything yet
void slideshow_init();

// Start or restart the slideshow
void slideshow_start();

// Reset the slideshow to the start of the loop
void slideshow_reset();

// Halt the slideshow
void slideshow_stop();

// Advance the display, showing whatever's next
void slideshow_next();

// Upload new measurement data for the slideshow to use
void slideshow_new_data(SnappySenseData* new_data);

// Set a message to be displayed immediately, for the normal period, and then erased.
// If the slideshow is not running then this does nothing.
void slideshow_show_message_once(String* msg);

#endif // !slideshow_h_included
