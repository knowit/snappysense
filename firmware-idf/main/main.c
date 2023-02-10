/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* SnappySense firmware code based on the ESP32-IDF + FreeRTOS framework only (no Arduino). */

#include "main.h"

#include <inttypes.h>
#include <math.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "factory_config.h"
#include "device.h"
#include "piezo.h"
#include "bitmaps.h"

/* Parameters */
#define MONITORING_WINDOW_S 10
#define MONITORING_INTERVAL_S 15
#define NEXT_SLIDE_INTERVAL_S 4

/* Generated by the music-compiler program (see repo root) */
/* StarWars */
const struct tone notes_5577006791947779410[] = {{1109,197},{740,197},{740,197},{740,197},{988,1183},{1480,1183},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1319,197},{1109,1183},{740,197},{740,197},{740,197},{988,1183},{1480,1183},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1319,197},{1109,789},{440,1578},};
const struct music melody = { 40, notes_5577006791947779410 };

/* Queues and clocks */
static QueueHandle_t evt_queue = NULL;
static TimerHandle_t sensor_clock = NULL;
static TimerHandle_t slideshow_clock = NULL;
static TimerHandle_t monitoring_clock = NULL;

static void clock_callback(TimerHandle_t t) {
  uint32_t ev = EV_NONE;
  if (t == sensor_clock) {
    ev = EV_SENSOR_CLOCK;
  } else if (t == slideshow_clock) {
    ev = EV_SLIDESHOW_CLOCK;
  } else if (t == monitoring_clock) {
    ev = EV_MONITORING_CLOCK;
  }
  xQueueSend(evt_queue, &ev, portMAX_DELAY);
}

/* Sensor state */
static bool have_temperature = false;
static float temperature;
static bool have_humidity = false;
static float humidity;
static bool have_atmospheric_pressure = false;
static unsigned atmospheric_pressure;
static bool have_uv_intensity = false;
static float uv_intensity;
static bool have_luminous_intensity = false;
static float luminous_intensity;
static bool have_calibrated_sen0514 = false;
static bool have_co2 = false;
static unsigned co2;
static bool have_tvoc = false;
static unsigned tvoc;
static bool have_aqi = false;
static unsigned aqi;
static bool motion = false;

/* Slideshow state */
static int slideshow_next = 0;
static bool show_debug = false;

static void advance_slideshow();
static void open_monitoring_window();
static void close_monitoring_window();
static void record_motion();
static void panic(const char* msg) __attribute__ ((noreturn));

void app_main(void)
{
  /* Bring the power line up and wait until it stabilizes */
  power_up_peripherals();

  /* Queue of events from clocks and ISRs to the main task */
  evt_queue = xQueueCreate(10, sizeof(uint32_t));
  install_interrupts(evt_queue);

  /* Initialize peripherals */

  /* Set up buttons but do not make them send events yet */
  initialize_onboard_buttons();

#ifdef SNAPPY_I2C
  if (!initialize_i2c()) {
    panic("i2c system inoperable; nothing will work");
  }
#endif

#ifdef SNAPPY_I2C_SSD1306
  /* Display */
  if (!initialize_i2c_ssd1306()) {
    LOG("Failed to init SSD1306");
  }
#endif

  /* We're far enough along that we can talk to the screen */
  LOG("Snappysense active!");

  /* Look for factory config signal */
  if (btn1_is_pressed()) {
    for ( int i=0 ; i < 10 && btn1_is_pressed(); i++ ) {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    if (btn1_is_pressed()) {
      show_text("SnappySense v1.1\nUSB config");
      factory_configuration();
      esp_restart();
    }
  }

#ifdef SNAPPY_GPIO_SEN0171
  /* Movement sensor */
  if (!initialize_gpio_sen0171()) {
    LOG("Movement sensor inoperable");
  }
#endif

  /* TODO: SNAPPY_ADC_SEN0487 microphone */

#ifdef SNAPPY_I2C_SEN0500
  /* Environment sensor */
  if (!initialize_i2c_sen0500()) {
    LOG("Environment sensor inoperable");
  }
#endif

#ifdef SNAPPY_I2C_SEN0514
  /* Air/gas sensor */
  if (!initialize_i2c_sen0514()) {
    LOG("Air/gas sensor inoperable");
  }
#endif

#ifdef SNAPPY_GPIO_PIEZO
  if (!initialize_gpio_piezo() || !piezo_begin()) {
    LOG("Piezo speaker inoperable");
  }
#endif

  /* Create a clock tick used to drive device readings */
  /* TODO: Should we have this instead separate windows by this interval and not
     just blindly start new monitoring this often? */
  sensor_clock = xTimerCreate("sensor", pdMS_TO_TICKS(MONITORING_INTERVAL_S*1000),
                              /* restart= */ pdTRUE,
                              NULL, clock_callback);
  xTimerStart(sensor_clock, portMAX_DELAY);

  /* This one-shot timer is started when the monitoring window opens and signals the
     close of the window. */
  monitoring_clock = xTimerCreate("monitor", pdMS_TO_TICKS(MONITORING_WINDOW_S*1000),
                                  /* restart= */ pdFALSE,
                                  NULL, clock_callback);

#ifdef SNAPPY_I2C_SSD1306
  /* Create a clock tick used to drive the slideshow, independent of monitoring. */
  if (ssd1306) {
    slideshow_clock = xTimerCreate("slideshow", pdMS_TO_TICKS(NEXT_SLIDE_INTERVAL_S*1000),
                                   /* restart= */ pdTRUE,
                                   NULL, clock_callback);
    xTimerStart(slideshow_clock, portMAX_DELAY);
  }
#endif

  /* Buttons will now send events */
  enable_onboard_buttons();

  /* And we are up! */
  LOG("Snappysense running!");
  show_text("SnappySense v1.1\nKnowIt ObjectNet\n2023-02-07 / IDF");
  //  play_song(&melody);

  /* Process events forever.

     This is mainly a state machine with a cycle of sleep-monitoring-reporting-sleep-... states.
     Within the monitoring and reporting states there may be minor cycles.  Right now the machine is
     driven by a clock that starts the monitoring state and there is no reporting state, but this
     will change.
   
     Overlapping that state machine is one that drives the slide show, if enabled.  The two state
     machines are independent but are handled by the same switch for practical reasons.  Thus some
     display work can happen during monitoring, but this should not adversely affect anything.
   
     Finally, some events can interrupt the state machine.  PIR interrupts are seen during
     monitoring and are simply recorded.  Button interrupts are seen anytime and are currently
     ignored but may cause the device operation to change. */

  struct timeval button_down; /* Time of button press */
  bool was_pressed = false;   /*   if this is true */

  for(;;) {
    uint32_t ev;
    if(xQueueReceive(evt_queue, &ev, portMAX_DELAY)) {
      switch (ev & 15) {
      case EV_PIR:
        record_motion();
	break;

      case EV_BTN1: {
	/* Experimentation suggests that it's possible to have spurious button presses of around 10K
	   us, when the finger nail sort of touches the edge of the button and slides off.  A "real"
	   press lasts at least 100K us. */
	uint32_t state = ev >> 4;
	LOG("BUTTON: %" PRIu32, state);
	if (!state && was_pressed) {
	  struct timeval now;
	  gettimeofday(&now, NULL);
	  uint64_t t = ((uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_usec) -
	    ((uint64_t)button_down.tv_sec * 1000000 + (uint64_t)button_down.tv_usec);
	  LOG("  Pressed for %" PRIu64 "us", t);
          if (t < 1000000) {
            show_debug = !show_debug;
          }
	}
	was_pressed = state == 1;
	if (was_pressed) {
	  gettimeofday(&button_down, NULL);
	}
	break;
      }

      case EV_SENSOR_CLOCK:
        open_monitoring_window();
        break;

      case EV_MONITORING_CLOCK:
        close_monitoring_window();
        break;

      case EV_SLIDESHOW_CLOCK:
        advance_slideshow();
        break;

      default:
	LOG("Unknown event: %" PRIu32, ev);
	break;
      }
    } else {
      /* WHAT TO DO HERE?  We timed out or there was some bizarre error? */
    }
  }
}

static void panic(const char* msg) {
  LOG("PANIC: %s", msg);
  show_text("PANIC: %s", msg);
  for(;;) {}
}

static void open_monitoring_window() {
  LOG("Monitoring window opens");
#ifdef SNAPPY_I2C_SEN0500
  /* Environment sensor.  These we read instantaneously.  It might make sense to read multiple times
     and average or otherwise integrate; TBD. */
  if (have_sen0500) {
    have_temperature =
      dfrobot_sen0500_get_temperature(&sen0500, DFROBOT_SEN0500_TEMP_C, &temperature) &&
      temperature != -45.0;
    if (have_temperature) {
      LOG("Temperature = %.2f", temperature);
    }
    have_humidity =
      dfrobot_sen0500_get_humidity(&sen0500, &humidity) &&
      humidity != 0.0;
    if (have_humidity) {
      LOG("Humidity = %.2f", humidity);
    }
    have_atmospheric_pressure =
      dfrobot_sen0500_get_atmospheric_pressure(&sen0500, DFROBOT_SEN0500_PRESSURE_HPA,
                                               &atmospheric_pressure) &&
      atmospheric_pressure != 0;
    if (have_atmospheric_pressure) {
      LOG("Pressure = %u", atmospheric_pressure);
    }
    have_uv_intensity =
      dfrobot_sen0500_get_ultraviolet_intensity(&sen0500, &uv_intensity) &&
      uv_intensity != 0.0;
    if (have_uv_intensity) {
      LOG("UV intensity = %.2f", uv_intensity);
    }
    have_luminous_intensity =
      dfrobot_sen0500_get_luminous_intensity(&sen0500, &luminous_intensity) &&
      luminous_intensity != 0.0;
    if (have_luminous_intensity) {
      LOG("Luminous intensity = %.2f", luminous_intensity);
    }
  }
#endif
#ifdef SNAPPY_I2C_SEN0514
  if (have_sen0514) {
    dfrobot_sen0514_status_t stat;
    /* TODO: It's far from clear *when* this calibration should occur - whether it's every time we
       want to read the device or just the first time, or when temp and humidity change "a lot".  */
    if (have_temperature && have_humidity && !have_calibrated_sen0514) {
      if (dfrobot_sen0514_prime(&sen0514, temperature, humidity/100.0f)) {
        have_calibrated_sen0514 = true;
      }
    }
    if (have_calibrated_sen0514 &&
        dfrobot_sen0514_get_sensor_status(&sen0514, &stat) &&
        /* See the header for an explanation of the status codes */
        stat != DFROBOT_SEN0514_INVALID_OUTPUT) {
      have_co2 =
        dfrobot_sen0514_get_co2(&sen0514, &co2) &&
        co2 > 400;
      if (have_co2) {
        LOG("CO2 = %u", co2);
      }
      have_tvoc =
        dfrobot_sen0514_get_total_volatile_organic_compounds(&sen0514, &tvoc) &&
        tvoc > 0;
      if (have_tvoc) {
        LOG("TVOC = %u", tvoc);
      }
      have_aqi =
        dfrobot_sen0514_get_air_quality_index(&sen0514, &aqi) &&
        aqi >= 1 && aqi <= 5;
      if (have_aqi) {
        LOG("AQI = %u", aqi);
      }
    } else {
      LOG("SEN0514 not ready: %d", stat);
    }
  }
#endif
#ifdef SNAPPY_GPIO_SEN0171
  /* Motion sensor.  We enable the interrupt for the PIR while the monitoring window is open; then
     PIR interrupts will simply be recorded higher up in the switch. */
  motion = false;
  enable_gpio_sen0171();
#endif
  xTimerStart(monitoring_clock, portMAX_DELAY);
}

static void record_motion() {
  LOG("Motion!");
  motion = true;
}

static void close_monitoring_window() {
#ifdef SNAPPY_GPIO_SEN0171
  disable_gpio_sen0171();
#endif
  LOG("Monitoring window closed");
}

#ifdef SNAPPY_I2C_SSD1306
static void splash_screen() {
  ssd1306_Fill(ssd1306, SSD1306_BLACK);
  ssd1306_DrawBitmap(ssd1306, 0, 1,
                     knowit_logo_bitmap, KNOWIT_LOGO_WIDTH, KNOWIT_LOGO_HEIGHT,
                     SSD1306_WHITE);
  ssd1306_UpdateScreen(ssd1306);
}
#endif

static void advance_slideshow() {
  switch (slideshow_next) {
  case 0:
    slideshow_next++;
#ifdef SNAPPY_I2C_SSD1306
    if (ssd1306) {
      splash_screen();
      break;
    }
#endif
    /* FALLTHROUGH */
  case 1:
    slideshow_next++;
    if (have_temperature) {
      show_text("Temperature\n\n%.1f C", temperature);
      break;
    }
    /* FALLTHROUGH */
  case 2:
    slideshow_next++;
    if (have_humidity) {
      show_text("Humidity\n\n%.1f %%", humidity);
      break;
    }
    /* FALLTHROUGH */
  case 3:
    slideshow_next++;
    if (have_atmospheric_pressure) {
      show_text("Pressure\n\n%u hPa", atmospheric_pressure);
      break;
    }
    /* FALLTHROUGH */
  case 4:
    slideshow_next++;
    if (have_uv_intensity) {
      show_text("UV index\n\n%d", (int)roundf(uv_intensity));
      break;
    }
    /* FALLTHROUGH */
  case 5:
    slideshow_next++;
    if (have_luminous_intensity) {
      show_text("Light\n\n%.1f lux", luminous_intensity);
      break;
    }
    /* FALLTHROUGH */
  case 6:
    slideshow_next++;
    if (have_co2) {
      const char* co2_text;
      /* The scale is from the data sheet */
      if (co2 <= 600) {
        co2_text = "excellent";
      } else if (co2 <= 800) {
        co2_text = "good";
      } else if (co2 <= 1000) {
        co2_text = "adequate";
      } else if (co2 <= 1500) {
        co2_text = "bad";
      } else {
        co2_text = "terrible";
      };
      show_text("CO_2\n\n%uppm - %s", co2, co2_text);
      break;
    }
    /* FALLTHROUGH */
  case 7:
    slideshow_next++;
    if (have_tvoc) {
      const char* tvoc_text;
      /* The scale is partly from the data sheet */
      if (tvoc < 50) {
        tvoc_text = "good";
      } else if (tvoc < 200) {
        tvoc_text = "adequate";
      } else if (tvoc < 750) {
        tvoc_text = "not great";
      } else if (tvoc < 6000) {
        tvoc_text = "bad";
      } else {
        tvoc_text = "dangerous";
      }
      show_text("Volatile organics\n\n%uppb - %s", tvoc, tvoc_text);
      break;
    }
    /* FALLTHROUGH */
  case 8:
    slideshow_next++;
    if (have_aqi) {
      /* The scale is from the data sheet */
      static const char* aqi_text[] = {
        "",
        "excellent",
        "good",
        "adequate",
        "bad",
        "terrible" };
      show_text("Air quality index\n\n%d - %s", aqi, aqi_text[aqi]);
      break;
    }
    /* FALLTHROUGH */
  case 9:
    slideshow_next++;
#ifdef SNAPPY_GPIO_SEN0171
    show_text("Movement\n\n%s", motion ? "Yes" : "No");
    break;
#else
    /* FALLTHROUGH */
#endif
  case 10:
    slideshow_next++;
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
      show_text(buf);
      break;
    }
    /* FALLTHROUGH */
  default:
    slideshow_next = 0;
    break;
  }
}

/* Display abstractions. */
#ifdef SNAPPY_I2C_SSD1306
void show_text(const char* fmt, ...) {
  if (!ssd1306) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  char buf[32*4];		/* Largest useful string for this screen and font */
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  ssd1306_Fill(ssd1306, SSD1306_BLACK);
  char* p = buf;
  int y = 0;
  while (*p) {
    char* start = p;
    while (*p && *p != '\n') {
      p++;
    }
    if (*p) {
      *p++ = 0;
    }
    ssd1306_SetCursor(ssd1306, 0, y);
    ssd1306_WriteString(ssd1306, start, Font_7x10, SSD1306_WHITE);
    y += Font_7x10.FontHeight + 1;
  }
  ssd1306_UpdateScreen(ssd1306);
}
#endif

#ifdef SNAPPY_LOGGING
void snappy_log(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  if (fmt[strlen(fmt)-1] != '\n') {
    putchar('\n');
  }
}
#endif

