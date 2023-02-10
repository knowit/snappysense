/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef device_h_included
#define device_h_included

#include "main.h"
#include "dfrobot_sen0500.h"
#include "dfrobot_sen0514.h"
#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void power_up_peripherals();
void install_interrupts(QueueHandle_t evt_queue);

void initialize_onboard_buttons();
void enable_onboard_buttons();
bool btn1_is_pressed();

#ifdef SNAPPY_GPIO_SEN0171
bool initialize_gpio_sen0171() WARN_UNUSED;
void enable_gpio_sen0171();
void disable_gpio_sen0171();
#endif

#ifdef SNAPPY_I2C
bool initialize_i2c() WARN_UNUSED;
#endif

#ifdef SNAPPY_I2C_SEN0500
extern bool have_sen0500;
extern dfrobot_sen0500_t sen0500;

/* Initialize the device, set have_sen0500 to true iff it succeeds. */
bool initialize_i2c_sen0500() WARN_UNUSED;
#endif

#ifdef SNAPPY_I2C_SEN0514
extern bool have_sen0514;
extern dfrobot_sen0514_t sen0514;

/* Initialize the device, set have_sen05514 to true iff it succeeds. */
bool initialize_i2c_sen0514() WARN_UNUSED;
#endif

#ifdef SNAPPY_I2C_SSD1306
extern uint8_t ssd1306_mem[];
extern SSD1306_Device_t* ssd1306;

/* Initialize the device, set ssd1306 to non-null iff it succeeds */
bool initialize_i2c_ssd1306() WARN_UNUSED;
#endif

#ifdef SNAPPY_GPIO_PIEZO
bool initialize_gpio_piezo() WARN_UNUSED;
void start_note(int frequency);
void stop_note();
#endif

#endif /* device_h_included */
