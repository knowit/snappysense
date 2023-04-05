/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* SnappySense firmware code based only on FreeRTOS, with esp32-idf at the device interface level.
   No Arduino. */

#ifndef main_h_included
#define main_h_included

/*
   CODING STANDARDS
 
   - All <filename>.c files shall have a corresponding <filename>.h and the <filename>.c file shall
     include <filename>.h first.

   - All <filename>.h shall include main.h first.

   - Always handle errors, including OOM conditions.  Use WARN_UNUSED on functions that return error
     indicators.  Benign / recoverable errors can be ignored / recovered from.  Everything else
     should end up calling panic() in main.c, one way or another.

   - Naming conventions: functions and variables and file names are snake_case, types are
     snake_case_t, global constants are ALL_UPPER_SNAKE_CASE.

   - Indent using spaces, not tabs.  Preferably 2 spaces per level.  Most code uses K&R style:
     opening brace on same line as controlling statement.  Most code is fully bracketed: even single
     dependent statements in `if`, `for`, `while`, `do` are enclosed in braces.

   - Try to conform to whatever standard is used in the file you're editing, if it differs (in
     reasonable ways) from the above.

   - Try to limit line lengths to 100 chars (allowing two side-by-side editors on laptops and three
     on most standalone displays)
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/timers.h"

/********************************************************************************
 *
 * General feature selection.
 */

#define SNAPPY_LOGGING          /* Debug logging to the USB */
#define SNAPPY_SLIDESHOW        /* Rotating display of data values, if enabled */
/*#define SNAPPY_SOUND_EFFECTS    / * Output sound to a speaker */
#define SNAPPY_OLED             /* Output on a screen */
#define SNAPPY_READ_TEMPERATURE
#define SNAPPY_READ_HUMIDITY
#define SNAPPY_READ_PRESSURE
#define SNAPPY_READ_UV_INTENSITY
#define SNAPPY_READ_LIGHT_INTENSITY
#define SNAPPY_READ_CO2
#define SNAPPY_READ_VOLATILE_ORGANICS
#define SNAPPY_READ_AIR_QUALITY_INDEX
#define SNAPPY_READ_MOTION
/*#define SNAPPY_READ_NOISE */

/********************************************************************************
 *
 * Hardware and devices.
 */

/*#define SNAPPY_HARDWARE_1_0_0*/
#define SNAPPY_HARDWARE_1_1_0

/* Environment sensor: DFRobot SEN0500
   https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor */
#define SNAPPY_I2C_SEN0500

/* Air/gas sensor: DFRobot SEN0514
   https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor */
#define SNAPPY_I2C_SEN0514

/* Sound sensor: DFRobot SEN0487 MEMS microphone
   https://wiki.dfrobot.com/Fermion_MEMS_Microphone_Sensor_SKU_SEN0487 */
/*
#define SNAPPY_ADC_SEN0487
*/

/* Movement sensor: DFRobot SEN0171 passive IR sensor, digital directly from GPIO
   https://wiki.dfrobot.com/PIR_Motion_Sensor_V1.0_SKU_SEN0171 */
#define SNAPPY_GPIO_SEN0171

/* OLED: SSD1306-based bitmapped display
   Eg https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/ */
#define SNAPPY_I2C_SSD1306
#ifdef SNAPPY_I2C_SSD1306
# define INCLUDE_FONT_7x10
/* # define INCLUDE_FONT_6x8 */
/* # define INCLUDE_FONT_11x18 */
/* # define INCLUDE_FONT_16x26 */
/* # define INCLUDE_FONT_16x24 */
/* # define FRAMEBUFFER_GRAPHICS */ /* Line and arc drawing primitives */
#endif

/* Piezo speaker: based on the ESP32-IDF "ledc" library */
/*
#define SNAPPY_ESP32_LEDC_PIEZO
*/

#if defined(SNAPPY_I2C_SEN0500) || defined(SNAPPY_I2C_SEN0514) || defined(SNAPPY_I2C_SSD1306)
# define SNAPPY_I2C
#endif

/********************************************************************************
 *
 * Check that devices are available for all the features that are selected.  This is overly
 * elaborate right now as we only have a single type of device for each feature, but it's clean and
 * some generalization is probably going to happen.
 */

#if defined(SNAPPY_SOUND_EFFECTS) && !defined(SNAPPY_ESP32_LEDC_PIEZO)
# error "SNAPPY_SOUND_EFFECTS without a sound device"
#endif

#if defined(SNAPPY_OLED) && !defined(SNAPPY_I2C_SSD1306)
# error "SNAPPY_OLED without an OLED device"
#endif

#if defined(SNAPPY_SLIDESHOW) && !defined(SNAPPY_OLED)
# error "SNAPPY_SLIDESHOW without an OLED device"
#endif

#if defined(SNAPPY_READ_TEMPERATURE) && !defined(SNAPPY_I2C_SEN0500)
# error "SNAPPY_READ_TEMPERATURE without a sensor device"
#endif

#if defined(SNAPPY_READ_HUMIDITY) && !defined(SNAPPY_I2C_SEN0500)
# error "SNAPPY_READ_HUMIDITY without a sensor device"
#endif

#if defined(SNAPPY_READ_PRESSURE) && !defined(SNAPPY_I2C_SEN0500)
# error "SNAPPY_READ_PRESSURE without a sensor device"
#endif

#if defined(SNAPPY_READ_UV_INTENSITY) && !defined(SNAPPY_I2C_SEN0500)
# error "SNAPPY_READ_UV_INTENSITY without a sensor device"
#endif

#if defined(SNAPPY_READ_LIGHT_INTENSITY) && !defined(SNAPPY_I2C_SEN0500)
# error "SNAPPY_READ_LIGHT_INTENSITY without a sensor device"
#endif

#if defined(SNAPPY_READ_CO2) && !defined(SNAPPY_I2C_SEN0514)
# error "SNAPPY_READ_CO2 without a sensor device"
#endif

#if defined(SNAPPY_READ_VOLATILE_ORGANICS) && !defined(SNAPPY_I2C_SEN0514)
# error "SNAPPY_READ_VOLATILE_ORGANICS without a sensor device"
#endif

#if defined(SNAPPY_READ_AIR_QUALITY_INDEX) && !defined(SNAPPY_I2C_SEN0514)
# error "SNAPPY_READ_AIR_QUALITY_INDEX without a sensor device"
#endif

#if defined(SNAPPY_READ_NOISE) && !defined(SNAPPY_ADC_SEN0487)
# error "SNAPPY_READ_NOISE without a sampler device"
#endif

#if defined(SNAPPY_READ_MOTION) && !defined(SNAPPY_GPIO_SEN0171)
# error "SNAPPY_READ_MOTION without a motion detector device"
#endif

#if defined(SNAPPY_I2C_SEN0514) && !(defined(SNAPPY_READ_TEMPERATURE) && defined(SNAPPY_READ_HUMIDITY))
# error "DFROBOT_SEN0514 device needs temperature and humidity for calibration"
#endif

/********************************************************************************
 *
 * Miscellaneous definitions.
 */

#define WARN_UNUSED __attribute__((warn_unused_result))
#define NO_RETURN __attribute__ ((noreturn))

#ifdef SNAPPY_LOGGING
extern void snappy_log(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
# define LOG(...) snappy_log(__VA_ARGS__)
#else
# define LOG(...)
#endif

/* Task priorities for helper tasks. */
#define PLAYER_PRIORITY 3

/* Event codes.  The events are all sent on the main event queue, sometimes with payloads, see
   put_main_event() functions below and the implementation in main.c. */
typedef enum {
  // Main task
  EV_START_CYCLE,
  EV_SLEEP_START,
  EV_POST_SLEEP,
  EV_MONITOR_START,
  EV_MONITOR_STOP,
  EV_MONITOR_DATA,
  EV_BUTTON_PRESS,

  // Monitor task
  EV_MONITOR_WARMUP,

  // Display+slideshow task
  EV_MESSAGE,
  EV_SLIDESHOW_RESET,
  EV_SLIDESHOW_START,
  EV_SLIDESHOW_STOP,
  EV_SLIDESHOW_WORK,

  // Button task
  EV_BUTTON_DOWN,               /* Payload: nothing.  Button was pressed. */
  EV_BUTTON_UP,                 /* Payload: nothing.  Button was released. */

  // Sensor task
  EV_MEMS_WORK,                 /* Payload: nothing.  Perform sound sampling. */
  EV_MEMS_SAMPLE,               /* Payload: sound level, 1..5. */
  EV_MOTION_DETECTED,           /* Payload: nothing.  From ISR. */
} snappy_event_t;

/* Put events into the event queue. */
void put_main_event(snappy_event_t ev);
void put_main_event_from_isr(snappy_event_t ev);

void put_main_event_with_ival(snappy_event_t ev, int val);

/* s points to statically allocated storage.  A heap copy will be made if necessary */
void put_main_event_with_string(snappy_event_t ev, const char* s);

/* data points to heap-allocated data, ownership is transfered */
typedef struct sensor_state sensor_state_t;
void put_main_event_with_data(snappy_event_t ev, sensor_state_t* data);

void panic(const char* msg) NO_RETURN;

#endif /* !main_h_included */
