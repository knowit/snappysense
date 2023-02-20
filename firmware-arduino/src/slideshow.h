#ifndef slideshow_h_included
#define slideshow_h_included

#include "main.h"
#include "sensor.h"

void slideshow_new_data(SnappySenseData* new_data);
void slideshow_next();
void slideshow_show_message_once(String* msg);

#endif // !slideshow_h_included
