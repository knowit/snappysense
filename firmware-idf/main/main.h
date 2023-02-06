/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef main_h_included
#define main_h_included

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define SNAPPY_LOGGING
//#define SNAPPY_HARDWARE_1_0_0
#define SNAPPY_HARDWARE_1_1_0

/* Environment sensor: DFRobot SEN0500
 * https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor
 */
#define SNAPPY_I2C_SEN0500

/* Air/gas sensor: DFRobot SEN0514
 * https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor
 */
#define SNAPPY_I2C_SEN0514

/* Sound sensor: DFRobot SEN0487 MEMS microphone
 * https://wiki.dfrobot.com/Fermion_MEMS_Microphone_Sensor_SKU_SEN0487
 */
//#define SNAPPY_ADC_SEN0487

/* Movement sensor: DFRobot SEN0171 passive IR sensor, digital directly from GPIO
 * https://wiki.dfrobot.com/PIR_Motion_Sensor_V1.0_SKU_SEN0171
 */
#define SNAPPY_GPIO_SEN0171

/* OLED: SSD1306-based 128x32 pixel display
 * Eg https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/
 */
#define SNAPPY_I2C_SSD1306
#ifdef SNAPPY_I2C_SSD1306
# define SSD1306_WIDTH 128
# define SSD1306_HEIGHT 32
# define SSD1306_INCLUDE_FONT_7x10 /* There are others, this is readable */
# define SSD1306_GRAPHICS
#endif

// TODO: DOCUMENTME
//#define SNAPPY_GPIO_PIEZO

#if defined(SNAPPY_I2C_SEN0500) || defined(SNAPPY_I2C_SEN0514) || defined(SNAPPY_I2C_SSD1306)
# define SNAPPY_I2C
#endif

#ifdef SNAPPY_LOGGING
# define LOG(...) printf(__VA_ARGS__)
#else
# define LOG(...)
#endif

/* Signal error and hang */
void panic(const char* msg) __attribute__ ((noreturn));

/* Events are uint32_t values sent from ISRs and monitoring tasks to
 * the main task, on a queue owned by the main task.  Numbers should
 * stay below 16.  Events have an event number in the low 4 bits and
 * payload in the upper 28.
 */
enum {
  EV_NONE,
  EV_PIR,	 /* Payload: pin level, 0 (no motion) or 1 (motion) */
  EV_BTN1,	 /* Payload: button level, 0 (up) or 1 (down) */
  EV_SENSOR_CLOCK,              /* Payload: nothing */
  EV_SLIDESHOW_CLOCK,            /* Payload: nothing */
  EV_MONITORING_CLOCK,
};

#endif /* !main_h_included */
