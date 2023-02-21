/* Manage the display - slideshow and error messages */

#include "slideshow.h"

#include "device.h"
#include "sensor.h"
#include "util.h"

// -1 means the splash screen; values 0..whatever refer to the entries
// in the SnappyMetaData array.
static int next_view = -1;
static SnappySenseData* current_data;
static SnappySenseData* next_data;
static String* current_message;

void slideshow_new_data(SnappySenseData* new_data) {
  assert(new_data != nullptr);
  delete next_data;
  next_data = new_data;
  //log("Got data\n");
  //log("%s\n", format_readings_as_json(*new_data).c_str());
}

// Show the message immediately, for a normal time slot (or until a new message arrives)
// The main loop will reset the slideshow timer after calling this fn, taking care
// of display.

void slideshow_show_message_once(String* msg) {
  assert(msg != nullptr);
  delete current_message;  // Overwrite another one that wasn't shown yet (for now)
  current_message = msg;
}

void slideshow_next() {
again:
  if (current_message != nullptr) {
    // Message pending.  Do not advance the pointer; deleting the message will
    // allow us to advance next time around
    render_text(current_message->c_str());
    delete current_message;
    current_message = nullptr;
    return;
  }

  if (next_view == -1) {
    // Display the splash, slot in new data, set error flags if needed
    show_splash();
    if (next_data != nullptr) {
      delete current_data;
      current_data = next_data;
      next_data = nullptr;
    }
    next_view++;
    return;
  }

  if (current_data == nullptr) {
    // No data, wrap around
    next_view = -1;
    goto again;
  }

  if (snappy_metadata[next_view].json_key == nullptr) {
    // At end of data, wrap around
    next_view = -1;
    goto again;
  }

  if (snappy_metadata[next_view].display == nullptr) {
    // Field is not for slide show display, try the next one
    next_view++;
    goto again;
  }

  if (snappy_metadata[next_view].flag_offset > 0 &&
      !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(current_data) + snappy_metadata[next_view].flag_offset)) {
    // Field has invalid data, try the next one
    next_view++;
    goto again;
  }

  // Valid field, display it and advance the pointer
  char buf[32];
  snappy_metadata[next_view].display(*current_data, buf, buf+sizeof(buf));
  render_oled_view(snappy_metadata[next_view].icon, buf, snappy_metadata[next_view].display_unit);
  next_view++;
}
