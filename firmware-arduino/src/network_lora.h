#ifndef network_lora_h_defined
#define network_lora_h_defined

#include "main.h"

#ifdef SNAPPY_LORA

#include "sensor.h"

void upload_add_data(SnappySenseData* new_data);
#endif

#endif // !network_lora_h_defined
