// Code for uploading data to a remote MQTT broker.

#ifndef mqtt_upload_h_included
#define mqtt_upload_h_included

#include "main.h"
#include "sensor.h"

#ifdef MQTT_UPLOAD
// Enqueue a startup message and initialize the mqtt state machine.
void start_mqtt();

// Perform MQTT work.  Returns the time to delay (in seconds) before
// calling this function again.  The function is robust: if there is
// no outgoing traffic and not enough time has passed since checking
// for incoming traffic, it does nothing, and returns the appropriate
// wait time.
unsigned long perform_mqtt_step();

// Create an mqtt message with current readings and enqeueue it.
void capture_readings_for_mqtt_upload(const SnappySenseData& data);
#endif

#endif // !mqtt_upload_h_included
