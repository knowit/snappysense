#ifndef slideshow_h_included
#define slideshow_h_included

#include "main.h"

#ifdef SLIDESHOW_MODE

#include "microtask.h"
#include "sensor.h"

class SlideshowTask final : public MicroTask {
  // -1 means the splash screen; values 0..whatever refer to the entries
  // in the SnappyMetaData array.
  int next_view = -1;
  bool wifi_ok = true;
  bool pending_error = false;
public:
  const char* name() override {
    return "Slideshow";
  }
  virtual bool only_when_device_enabled() {
    return true;
  }
  void execute(SnappySenseData* data) override;
  void setWiFiStatus(bool wifi_ok) {
    this->wifi_ok = wifi_ok;
  }
};

extern SlideshowTask* slideshow_task;
#endif

#endif // !slideshow_h_included
