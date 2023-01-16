// Code for uploading data to a remote MQTT broker.

#ifndef mqtt_upload_h_included
#define mqtt_upload_h_included

#include "main.h"

#ifdef MQTT_UPLOAD
#include "sensor.h"

// A task that sets the mqtt capture interval.
class SetMqttIntervalTask final : public MicroTask {
  unsigned interval_s;
public:
  SetMqttIntervalTask(unsigned interval_s) : interval_s(interval_s) {}
  const char* name() override {
    return "MQTT set interval";
  }
  void execute(SnappySenseData*) override;
};

// This spins up the Mqtt communication infrastructure and enqueues 
// any necessary startup messages.
class StartMqttTask final : public MicroTask {
public:
  const char* name() override {
    return "MQTT start";
  }
  void execute(SnappySenseData*) override;
};

// This handles communication with the MQTT broker.  It is scheduled one-shot,
// but reschedules itself appropriately for its state.
class MqttCommsTask final : public MicroTask {
  bool first_time = true;       // true for first connection
  unsigned long next_work = 0;  // millisecond timestamp
  unsigned long last_work = 0;  // millisecond timestamp

  bool connect();
  void disconnect();
  void send();
  bool poll();

public:
  const char* name() override {
    return "MQTT comms";
  }
  void execute(SnappySenseData*) override;
};

extern TaskHandle_t mqtt_capture_task_handle;
void mqtt_capture_task(void* parameter /* SnappySenseData* */);

#endif // MQTT_UPLOAD

#endif // !mqtt_upload_h_included
