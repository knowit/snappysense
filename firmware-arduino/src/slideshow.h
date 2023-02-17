#ifndef slideshow_h_included
#define slideshow_h_included

#include "main.h"

#include "microtask.h"
#include "sensor.h"
#include "util.h"

class SlideshowTask final : public MicroTask {
  // -1 means the splash screen; values 0..whatever refer to the entries
  // in the SnappyMetaData array.
  int next_view = -1;
  bool wifi_ok = true;
  bool pending_error = false;
public:
  // A singleton periodic task.
  static SlideshowTask* handle;

  SlideshowTask() {
    if (handle != nullptr) {
      panic("The slideshow-task is a singleton");
    }
    handle = this;
  }

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

#endif // !slideshow_h_included
