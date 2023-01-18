#include "slideshow.h"

#ifdef SLIDESHOW_MODE

#include "device.h"

SlideshowTask* slideshow_task;

// This state machine is a little messy since -1 is special and pending_error
// is special.  The idea, anyway, is that after the splash screen we can
// display pending errors.  Thus during slideshow operations any error display
// for a recoverable error is integrated in the slideshow.

void SlideshowTask::execute(SnappySenseData* data) {
  bool done = false;
  while (!done) {
    if (next_view == -1) {
      show_splash();
      if (!wifi_ok) {
        pending_error = true;
      }
      done = true;
    } else if (next_view == 0 && pending_error) {
      if (!wifi_ok) {
        render_text("No WiFi");
      }
      pending_error = false;
      next_view--;  // To offset the increment below
      done = true;
    } else if (snappy_metadata[next_view].json_key == nullptr) {
      // At end, wrap around
      next_view = -1;
    } else if (snappy_metadata[next_view].display == nullptr) {
      // Field is not for SLIDESHOW_MODE display
      next_view++;
    } else {
      char buf[32];
      snappy_metadata[next_view].display(*data, buf, buf+sizeof(buf));
      render_oled_view(snappy_metadata[next_view].icon, buf, snappy_metadata[next_view].display_unit);
      done = true;
    }
  }
  next_view++;
}
#endif // SLIDESHOW_MODE
