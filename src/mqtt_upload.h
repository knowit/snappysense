// Code for uploading data to a remote MQTT broker.
//
// larhan@knowit.no

#ifndef mqtt_upload_h_included
#define mqtt_upload_h_included

#include "main.h"
#include "sensor.h"

#ifdef MQTT_UPLOAD
void upload_results_to_mqtt_server(const SnappySenseData& data);
#endif

#endif // !mqtt_upload_h_included
