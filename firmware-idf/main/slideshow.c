/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "slideshow.h"

#include <math.h>
#include <string.h>
#include "device.h"
#include "oled.h"
#include "sensor.h"

#ifdef SNAPPY_SLIDESHOW
#define NEXT_SLIDE_INTERVAL_S 4

/* Slideshow state */
static int slide_index = 0;
static bool show_debug = false;

/* Clocks used by the main loop */
static TimerHandle_t slideshow_clock = NULL;

static void slideshow_clock_callback(TimerHandle_t t) {
  snappy_event_t ev = EV_SLIDESHOW_CLOCK;
  xQueueSend(snappy_event_queue, &ev, portMAX_DELAY);
}

void slideshow_begin() {  
  if (oled_present()) {
    /* Create a clock tick used to drive the slideshow, independent of monitoring. */
    slideshow_clock = xTimerCreate("slideshow", pdMS_TO_TICKS(NEXT_SLIDE_INTERVAL_S*1000),
                                   /* restart= */ pdTRUE,
                                   NULL, slideshow_clock_callback);
    xTimerStart(slideshow_clock, portMAX_DELAY);
  }
}

void slideshow_toggle_debug() {
  show_debug = !show_debug;
}

void slideshow_next() {
  switch (slide_index) {
  case 0:
    slide_index++;
    oled_splash_screen();
    break;
    /* FALLTHROUGH */
  case 1:
    slide_index++;
    if (sensor.have_temperature) {
      oled_show_text("Temperature\n\n%.1f C", sensor.temperature);
      break;
    }
    /* FALLTHROUGH */
  case 2:
    slide_index++;
    if (sensor.have_humidity) {
      oled_show_text("Humidity\n\n%.1f %%", sensor.humidity);
      break;
    }
    /* FALLTHROUGH */
  case 3:
    slide_index++;
    if (sensor.have_atmospheric_pressure) {
      oled_show_text("Pressure\n\n%u hPa", sensor.atmospheric_pressure);
      break;
    }
    /* FALLTHROUGH */
  case 4:
    slide_index++;
    if (sensor.have_uv_intensity) {
      oled_show_text("UV index\n\n%d", (int)roundf(sensor.uv_intensity));
      break;
    }
    /* FALLTHROUGH */
  case 5:
    slide_index++;
    if (sensor.have_luminous_intensity) {
      oled_show_text("Light\n\n%.1f lux", sensor.luminous_intensity);
      break;
    }
    /* FALLTHROUGH */
  case 6:
    slide_index++;
    if (sensor.have_co2) {
      const char* co2_text;
      /* The scale is from the data sheet */
      if (sensor.co2 <= 600) {
        co2_text = "excellent";
      } else if (sensor.co2 <= 800) {
        co2_text = "good";
      } else if (sensor.co2 <= 1000) {
        co2_text = "adequate";
      } else if (sensor.co2 <= 1500) {
        co2_text = "bad";
      } else {
        co2_text = "terrible";
      };
      oled_show_text("CO_2\n\n%uppm - %s", sensor.co2, co2_text);
      break;
    }
    /* FALLTHROUGH */
  case 7:
    slide_index++;
    if (sensor.have_tvoc) {
      const char* tvoc_text;
      /* The scale is partly from the data sheet */
      if (sensor.tvoc < 50) {
        tvoc_text = "good";
      } else if (sensor.tvoc < 200) {
        tvoc_text = "adequate";
      } else if (sensor.tvoc < 750) {
        tvoc_text = "not great";
      } else if (sensor.tvoc < 6000) {
        tvoc_text = "bad";
      } else {
        tvoc_text = "dangerous";
      }
      oled_show_text("Volatile organics\n\n%uppb - %s", sensor.tvoc, tvoc_text);
      break;
    }
    /* FALLTHROUGH */
  case 8:
    slide_index++;
    if (sensor.have_aqi) {
      /* The scale is from the data sheet */
      static const char* aqi_text[] = {
        "",
        "excellent",
        "good",
        "adequate",
        "bad",
        "terrible" };
      oled_show_text("Air quality index\n\n%d - %s", sensor.aqi, aqi_text[sensor.aqi]);
      break;
    }
    /* FALLTHROUGH */
  case 9:
    slide_index++;
#ifdef SNAPPY_READ_MOTION
    oled_show_text("Movement\n\n%s", sensor.motion ? "Yes" : "No");
    break;
#else
    /* FALLTHROUGH */
#endif
  case 10:
    slide_index++;
#ifdef SNAPPY_READ_SOUND
    if (have_sound_level) {
      /* The scale is made-up */
      static const char* sound_text[] = {
        "",
        "eerie",
        "quiet",
        "normal",
        "bad",
        "runway?" };
      oled_show_text("Sound level\n\n%d - %s", sensor.sound_level, sound_text[sensor.sound_level]);
      break;
    }
#endif
    /* FALLTHROUGH */
  case 11:
    slide_index++;
    if (show_debug) {
      char buf[128];
      *buf = 0;
#ifdef SNAPPY_I2C_SEN0514
      if (have_sen0514) {
        dfrobot_sen0514_status_t stat;
        if (dfrobot_sen0514_get_sensor_status(&sen0514, &stat)) {
          sprintf(buf + strlen(buf), "A=%d %c ", (int)stat, have_calibrated_sen0514 ? 'y' : 'n');
        } else {
          sprintf(buf + strlen(buf), "A=- %c ", have_calibrated_sen0514 ? 'y' : 'n');
        }
      }
#endif
      oled_show_text(buf);
      break;
    }
    /* FALLTHROUGH */
  default:
    slide_index = 0;
    break;
  }
}
#endif  /* SNAPPY_SLIDESHOW */
