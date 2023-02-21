// Code for uploading data to a remote MQTT broker.

#ifndef mqtt_upload_h_included
#define mqtt_upload_h_included

#include "main.h"

#ifdef MQTT_UPLOAD
#include "sensor.h"

// Takes ownership of the data
void mqtt_add_data(SnappySenseData* new_data);

// The wifi must be up.  Connect to the MQTT server, and
void mqtt_start();
bool have_mqtt_work();
void mqtt_stop();
void mqtt_work();
void mqtt_init();

#endif // MQTT_UPLOAD

#endif // !mqtt_upload_h_included
