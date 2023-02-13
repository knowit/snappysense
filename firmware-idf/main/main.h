/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

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

/********************************************************************************
 *
 * General feature selection.
 */

#define SNAPPY_LOGGING          /* Debug logging to the USB */
#define SNAPPY_SOUND_EFFECTS    /* Output sound to a speaker */
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
#define SNAPPY_READ_NOISE

/********************************************************************************
 *
 * Hardware and devices.
 */

//#define SNAPPY_HARDWARE_1_0_0
#define SNAPPY_HARDWARE_1_1_0

/* Environment sensor: DFRobot SEN0500
   https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor */
#define SNAPPY_I2C_SEN0500

/* Air/gas sensor: DFRobot SEN0514
   https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor */
#define SNAPPY_I2C_SEN0514

/* Sound sensor: DFRobot SEN0487 MEMS microphone
   https://wiki.dfrobot.com/Fermion_MEMS_Microphone_Sensor_SKU_SEN0487 */
#define SNAPPY_ADC_SEN0487

/* Movement sensor: DFRobot SEN0171 passive IR sensor, digital directly from GPIO
   https://wiki.dfrobot.com/PIR_Motion_Sensor_V1.0_SKU_SEN0171 */
#define SNAPPY_GPIO_SEN0171

/* OLED: SSD1306-based 128x32 pixel display
   Eg https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/ */
#define SNAPPY_I2C_SSD1306
#ifdef SNAPPY_I2C_SSD1306
/* Mirror the screen if needed.

   TODO: These should be parameters to the device init, not global defines. */
/*
# define SSD1306_MIRROR_VERT
# define SSD1306_MIRROR_HORIZ
*/

/* If your screen horizontal axis does not start in column 0 you can use this define to adjust the
   horizontal offset.

   TODO: This is not supported in the code, plus it should be in the device or in the framebuffer. */
/*
# define SSD1306_X_OFFSET
*/

/* Set inverse color if needed.

   TODO: This should be a parameter to the device init, not a global define. */
/*
# define SSD1306_INVERSE_COLOR
*/

/* Include only needed fonts */
# define INCLUDE_FONT_7x10
/*
# define INCLUDE_FONT_6x8
# define INCLUDE_FONT_11x18
# define INCLUDE_FONT_16x26
# define INCLUDE_FONT_16x24
*/

/* Include graphics primitives */
/*
# define FRAMEBUFFER_GRAPHICS
*/
#endif

/* TODO: DOCUMENTME */
#define SNAPPY_GPIO_PIEZO

#if defined(SNAPPY_I2C_SEN0500) || defined(SNAPPY_I2C_SEN0514) || defined(SNAPPY_I2C_SSD1306)
# define SNAPPY_I2C
#endif

/********************************************************************************
 *
 * Check that devices are available for all the features that are selected.  This is a little
 * elaborate right now as we only have a single type of device for each feature, but it's clean and
 * some generalization is probably going to happen.
 */

#if defined(SNAPPY_SOUND_EFFECTS) && !defined(SNAPPY_GPIO_PIEZO)
# error "SNAPPY_SOUND_EFFECTS without a sound device"
#endif

#if defined(SNAPPY_OLED) && !defined(SNAPPY_I2C_SSD1306)
# error "SNAPPY_OLED without an OLED device"
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

/* Events are uint32_t values sent from ISRs and monitoring tasks to the main task, on a queue owned
   by the main task.  Numbers should stay below 16.  Events have an event number in the low 4 bits
   and payload in the upper 28. */
enum {
  EV_NONE,
  EV_MOTION,	                /* Payload: nothing, this means "motion detected" */
  EV_BTN1,	                /* Payload: button level, 0 (up) or 1 (down) */
  EV_SENSOR_CLOCK,              /* Payload: nothing, this means "clock tick" */
  EV_SLIDESHOW_CLOCK,           /* Payload: nothing, this means "clock tick" */
  EV_MONITORING_CLOCK,          /* Payload: nothing, this means "clock tick" */
  EV_SOUND_SAMPLE,              /* Payload: sound level, 1..5 */
};
typedef uint32_t snappy_event_t; /* EV_ code in low 4 bits, payload in high bits */

/* Queue of events from buttons, clocks, samplers, and other peripherals to the main task. */
extern QueueHandle_t/*<snappy_event_t>*/ snappy_event_queue;

#define WARN_UNUSED __attribute__((warn_unused_result))

#ifdef SNAPPY_LOGGING
extern void snappy_log(const char* fmt, ...);
# define LOG(...) snappy_log(__VA_ARGS__)
#else
# define LOG(...)
#endif

#define SAMPLER_PRIORITY 3
#define PLAYER_PRIORITY 3

#endif /* !main_h_included */
