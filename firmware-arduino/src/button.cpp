// Implement button logic from raw down/up events.

#include "button.h"

#define DEBOUNCE_MS 100
#define SHORT_PRESS_MAX 1999
#define LONG_PRESS_MIN 3000

static bool button_is_down;
static struct timeval button_down_time;
static TimerHandle_t button_timer;

void button_init() {
  button_timer = xTimerCreate("button",
                              pdMS_TO_TICKS(LONG_PRESS_MIN),
                              pdFALSE,
                              nullptr,
                              [](TimerHandle_t t) {
                                if (button_is_down) {
                                  button_is_down = false;
                                  put_main_event(EvCode::BUTTON_LONG_PRESS);
                                }
                              });
}

void button_down() {
  if (button_timer == nullptr) {
    return;
  }
  // ISR is signaling that the button is down. `button_down_time` is the time we pressed it.
  button_is_down = true;
  gettimeofday(&button_down_time, nullptr);
  xTimerReset(button_timer, portMAX_DELAY);
}

void button_up() {
  if (!button_is_down) {
    return;
  }
  xTimerStop(button_timer, portMAX_DELAY);
  button_is_down = false;
  struct timeval now;
  gettimeofday(&now, nullptr);
  uint64_t press_ms = ((uint64_t(now.tv_sec) * 1000000 + now.tv_usec) -
                       (uint64_t(button_down_time.tv_sec) * 1000000 + button_down_time.tv_usec)) / 1000;
  if (press_ms > DEBOUNCE_MS && press_ms <= SHORT_PRESS_MAX) {
    put_main_event(EvCode::BUTTON_PRESS);
  } else if (press_ms >= LONG_PRESS_MIN) {
    put_main_event(EvCode::BUTTON_LONG_PRESS);
  }
}
