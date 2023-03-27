// Code for uploading data to a remote MQTT broker and receiving config and
// command messages from it.

#ifndef mqtt_h_included
#define mqtt_h_included

#include "main.h"

#ifdef SNAPPY_MQTT

#include "sensor.h"

void mqtt_init();

// Takes ownership of the data
void upload_add_data(SnappySenseData* new_data);

// The wifi must be up.  Connect to the MQTT server, and ...
void mqtt_start();

bool mqtt_have_work();

// Can be called without start having been called first.
void mqtt_stop();

void mqtt_work();

#endif // SNAPPY_MQTT

#endif // !mqtt_h_included
