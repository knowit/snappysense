#include "slideshow.h"

#ifdef SLIDESHOW_MODE

#include "device.h"

void SlideshowTask::execute(SnappySenseData* data) {
  bool done = false;
  while (!done) {
    if (next_view == -1) {
      show_splash();
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
