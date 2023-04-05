/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "slideshow.h"

#include <math.h>
#include <string.h>
#include "device.h"
#include "oled.h"
#include "sensor.h"

#ifdef SNAPPY_SLIDESHOW
#define NEXT_SLIDE_INTERVAL_S 2 /* FIXME - should be config function */

/* Slideshow state */
static int slide_index;

/* Clocks used by the main loop */
static TimerHandle_t slideshow_clock;

static sensor_state_t* current_data;
static sensor_state_t* next_data;

static char* message;

static void slideshow_clock_callback(TimerHandle_t t) {
  put_main_event(EV_SLIDESHOW_WORK);
}

bool slideshow_begin() {  
  slideshow_clock = xTimerCreate("slideshow", pdMS_TO_TICKS(NEXT_SLIDE_INTERVAL_S*1000),
                                 /* restart= */ pdTRUE,
                                 NULL, slideshow_clock_callback);
  return slideshow_clock != NULL;
}

void slideshow_start() {
  xTimerStart(slideshow_clock, portMAX_DELAY);
}

void slideshow_stop() {
  xTimerStop(slideshow_clock, portMAX_DELAY);
}

void slideshow_reset() {
  slide_index = 0;
}

void slideshow_new_data(sensor_state_t* data) {
  if (current_data == NULL) {
    assert(next_data == NULL);
    current_data = data;
  } else {
    if (next_data != NULL) {
      free(next_data);
    }
    next_data = data;
  }
}

void slideshow_show_message_once(char* msg) {
  free(message);
  message = msg;
}

void slideshow_next() {
  if (message) {
    oled_show_text("%s", message);
    free(message);
    message = NULL;
    return;
  }

  switch (slide_index) {
  case 0:
    slide_index++;
    oled_splash_screen();
    if (next_data != NULL) {
      free(current_data);
      current_data = next_data;
      next_data = NULL;
    }
    break;
    /* FALLTHROUGH */
  case 1:
    slide_index++;
    if (current_data && current_data->have_temperature) {
      oled_show_text("Temperature\n\n%.1f C", current_data->temperature);
      break;
    }
    /* FALLTHROUGH */
  case 2:
    slide_index++;
    if (current_data && current_data->have_humidity) {
      oled_show_text("Humidity\n\n%.1f %%", current_data->humidity);
      break;
    }
    /* FALLTHROUGH */
  case 3:
    slide_index++;
    if (current_data && current_data->have_atmospheric_pressure) {
      oled_show_text("Pressure\n\n%u hPa", current_data->atmospheric_pressure);
      break;
    }
    /* FALLTHROUGH */
  case 4:
    slide_index++;
    if (current_data && current_data->have_uv_intensity) {
      oled_show_text("UV index\n\n%d", (int)roundf(current_data->uv_intensity));
      break;
    }
    /* FALLTHROUGH */
  case 5:
    slide_index++;
    if (current_data && current_data->have_luminous_intensity) {
      oled_show_text("Light\n\n%.1f lux", current_data->luminous_intensity);
      break;
    }
    /* FALLTHROUGH */
  case 6:
    slide_index++;
    if (current_data && current_data->have_co2) {
      const char* co2_text;
      /* The scale is from the data sheet */
      if (current_data->co2 <= 600) {
        co2_text = "excellent";
      } else if (current_data->co2 <= 800) {
        co2_text = "good";
      } else if (current_data->co2 <= 1000) {
        co2_text = "adequate";
      } else if (current_data->co2 <= 1500) {
        co2_text = "bad";
      } else {
        co2_text = "terrible";
      };
      oled_show_text("CO_2\n\n%uppm - %s", current_data->co2, co2_text);
      break;
    }
    /* FALLTHROUGH */
  case 7:
    slide_index++;
    if (current_data && current_data->have_tvoc) {
      const char* tvoc_text;
      /* The scale is partly from the data sheet */
      if (current_data->tvoc < 50) {
        tvoc_text = "good";
      } else if (current_data->tvoc < 200) {
        tvoc_text = "adequate";
      } else if (current_data->tvoc < 750) {
        tvoc_text = "not great";
      } else if (current_data->tvoc < 6000) {
        tvoc_text = "bad";
      } else {
        tvoc_text = "dangerous";
      }
      oled_show_text("Volatile organics\n\n%uppb - %s", current_data->tvoc, tvoc_text);
      break;
    }
    /* FALLTHROUGH */
  case 8:
    slide_index++;
    if (current_data && current_data->have_aqi) {
      /* The scale is from the data sheet */
      static const char* aqi_text[] = {
        "",
        "excellent",
        "good",
        "adequate",
        "bad",
        "terrible" };
      oled_show_text("Air quality index\n\n%d - %s", current_data->aqi, aqi_text[current_data->aqi]);
      break;
    }
    /* FALLTHROUGH */
  case 9:
    slide_index++;
#ifdef SNAPPY_READ_MOTION
    if (current_data) {
      oled_show_text("Movement\n\n%s", current_data->motion ? "Yes" : "No");
      break;
    }
#endif
    /* FALLTHROUGH */
  case 10:
    slide_index++;
#ifdef SNAPPY_READ_SOUND
    if (current_data && current_data->have_sound_level) {
      /* The scale is made-up */
      static const char* sound_text[] = {
        "",
        "eerie",
        "quiet",
        "normal",
        "bad",
        "runway?" };
      oled_show_text("Sound level\n\n%d - %s", current_data->sound_level, sound_text[current_data->sound_level]);
      break;
    }
#endif
    /* FALLTHROUGH */
  case 11:
    slide_index++;
    /* FALLTHROUGH */
  default:
    slide_index = 0;
    break;
  }
}

#endif  /* SNAPPY_SLIDESHOW */
