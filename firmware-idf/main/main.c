/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* SnappySense firmware code based only on FreeRTOS, with esp32-idf at the device interface level.
   No Arduino. */

#include "main.h"

#include <inttypes.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include "device.h"
#include "sound_player.h"
#include "sound_sampler.h"
#include "oled.h"
#include "slideshow.h"
#include "sensor.h"

/* Generated by the util/music-compiler program (see repo root) */
/* StarWars */
const struct tone notes_5577006791947779410[] = {{1109,197},{740,197},{740,197},{740,197},{988,1183},{1480,1183},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1319,197},{1109,1183},{740,197},{740,197},{740,197},{988,1183},{1480,1183},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1109,197},{1976,1183},{1480,591},{1319,197},{1245,197},{1319,197},{1109,789},{440,1578},};
const struct music melody = { 40, notes_5577006791947779410 };

/* The global event queue for the main loop */
QueueHandle_t/*<snappy_event_t>*/ snappy_event_queue = NULL;

static void handle_button(uint32_t state);
static void panic(const char* msg) NO_RETURN;
static void snappy_main() NO_RETURN;

void app_main(void) {
  /* Everyone uses this queue, so make sure it's here. */
  snappy_event_queue = xQueueCreate(10, sizeof(snappy_event_t));
  if (snappy_event_queue == NULL) {
    panic("Could not create event queue");
  }

  /*******************************************************************************
   *
   * Boot level 1: Devices that must work */

  /* Bring the power line up and wait until it stabilizes */
  power_up_peripherals();

  /* Set up interrupt handling, interrupts will be hooked by and by. */
  install_interrupts();

  /* Peripherals */

  /* Set up buttons but do not make them send events yet */
  initialize_onboard_buttons();

#ifdef SNAPPY_I2C
  if (!initialize_i2c()) {
    panic("i2c initialization failed; nothing will work");
  }
#endif

#ifdef SNAPPY_I2C_SSD1306
  /* Display */
  if (!initialize_i2c_ssd1306()) {
    LOG("OLED device inoperable");
  }
#endif
#ifdef SNAPPY_OLED
  oled_clear_screen();
#endif

  /* We're far enough along that we can talk to the screen */
  LOG("Snappysense active!");
#ifdef SNAPPY_OLED
  oled_show_text("SnappySense v1.1\nKnowIt ObjectNet\n2023-02-07 / IDF");
#endif

  /*******************************************************************************
   *
   * Boot level 2: Sensor devices, these can be absent */

#ifdef SNAPPY_GPIO_SEN0171
  /* Movement sensor */
  if (!initialize_gpio_sen0171()) {
    LOG("Movement device inoperable");
  }
#endif

#ifdef SNAPPY_ADC_SEN0487
  /* Sound sensor */
  if (!initialize_adc_sen0487()) {
    LOG("Sound device inoperable");
  }
#endif

#ifdef SNAPPY_I2C_SEN0500
  /* Environment sensor */
  if (!initialize_i2c_sen0500()) {
    LOG("Environment device inoperable");
  }
#endif

#ifdef SNAPPY_I2C_SEN0514
  /* Air/gas sensor */
  if (!initialize_i2c_sen0514()) {
    LOG("Air/gas device inoperable");
  }
#endif

#ifdef SNAPPY_ESP32_LEDC_PIEZO
  if (!initialize_esp32_ledc_piezo()) {
    LOG("Piezo device inoperable");
  }
#endif

  /*******************************************************************************
   *
   * Boot level 3: helper tasks and clocks.
   *
   * TODO: Clock creation must not fail, check that. */

#ifdef SNAPPY_READ_NOISE
  if (!sound_sampler_begin()) {
    LOG("Unable to initialize the sampler task");
  }
#endif
#ifdef SNAPPY_SOUND_EFFECTS
  if (!sound_effects_begin()) {
    LOG("Unable to initialize the sound player task");
  }
#endif

  if (!sensor_begin()) {
    LOG("Could not start sensor subsystem");
    panic("Sensor fail");
  }

  /* Buttons will now send events */
  enable_onboard_buttons();

  snappy_main();
}

static void snappy_main() {
  LOG("Snappysense running!");

#ifdef SNAPPY_SLIDESHOW
  slideshow_begin();
#endif

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

  for(;;) {
    uint32_t ev;
    if(xQueueReceive(snappy_event_queue, &ev, portMAX_DELAY)) {
      switch (ev & 15) {
      case EV_BTN1:
        handle_button(ev >> 4);
	break;

      case EV_SENSOR_CLOCK:
        open_monitoring_window();
        break;

      case EV_MONITORING_CLOCK:
        close_monitoring_window();
        break;

#ifdef SNAPPY_SLIDESHOW
      case EV_SLIDESHOW_CLOCK:
        slideshow_next();
        break;
#endif

#ifdef SNAPPY_READ_MOTION
      case EV_MOTION:
        record_motion();
	break;
#endif

#ifdef SNAPPY_READ_NOISE
      case EV_SOUND_SAMPLE:
        record_noise(ev >> 4);
        break;
#endif

      default:
	LOG("Unknown event: %" PRIu32, ev);
	break;
      }
    } else {
      /* Timeout.  Just try again */
    }
  }
}

static void panic(const char* msg) {
  LOG("PANIC: %s", msg);
#ifdef SNAPPY_OLED
  oled_show_text("PANIC: %s", msg);
#endif
  for(;;) {}
}

static struct timeval button_down; /* Time of button press */
static bool was_pressed = false;   /*   if this is true */

static void handle_button(uint32_t state) {
  /* Experimentation suggests that it's possible to have spurious button presses of around 10K
     us, when the finger nail sort of touches the edge of the button and slides off.  A "real"
     press lasts at least 100K us. */
  LOG("BUTTON: %" PRIu32, state);
  if (!state && was_pressed) {
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t t = ((uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_usec) -
      ((uint64_t)button_down.tv_sec * 1000000 + (uint64_t)button_down.tv_usec);
    LOG("  Pressed for %" PRIu64 "us", t);
    if (t < 1000000) {
#ifdef SNAPPY_SLIDESHOW
      slideshow_toggle_debug();
#endif
    }
  }
  was_pressed = state == 1;
  if (was_pressed) {
    gettimeofday(&button_down, NULL);
  }
}

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

