// Code for uploading data to a remote MQTT broker.

#ifndef mqtt_upload_h_included
#define mqtt_upload_h_included

#include "main.h"

#ifdef MQTT_UPLOAD
#include "sensor.h"

// Perform MQTT work.  Returns the time to delay (in seconds) before
// calling this function again.  The function is robust: if there is
// no outgoing traffic and not enough time has passed since checking
// for incoming traffic, it does nothing, and returns the appropriate
// wait time.
//unsigned long perform_mqtt_step();

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

// This captures the data from the sensor model into a message which it
// then enqueues for upload by the comms task.
class CaptureSensorsForMqttTask final : public MicroTask {
public:
  const char* name() override {
    return "MQTT capture";
  }
  virtual bool only_when_device_enabled() {
    return true;
  }
  void execute(SnappySenseData*) override;
};
#endif // MQTT_UPLOAD

#endif // !mqtt_upload_h_included
