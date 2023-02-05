#ifndef device_h_included
#define device_h_included

#include "main.h"
#include "dfrobot_sen0500.h"
#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void power_up_peripherals();
void install_interrupts(QueueHandle_t evt_queue);

void install_buttons();

#ifdef SNAPPY_GPIO_SEN0171
void initialize_gpio_sen0171();
void enable_gpio_sen0171();
void disable_gpio_sen0171();
#endif

#ifdef SNAPPY_I2C
void initialize_i2c();
#endif

#ifdef SNAPPY_I2C_SEN0500
extern bool have_sen0500;
extern dfrobot_sen0500_t sen0500;

/* Initialize the device, set have_sen0500 to true iff it succeeds. */
void initialize_i2c_sen0500();
#endif

#ifdef SNAPPY_I2C_SSD1306
extern uint8_t ssd1306_mem[];
extern SSD1306_Device_t* ssd1306;

/* Initialize the device, set ssd1306 to non-null iff it succeeds */
void initialize_i2c_ssd1306();
#endif

#ifdef SNAPPY_GPIO_PIEZO
void setup_sound();
void start_note(int frequency);
void stop_note();
#endif

#endif /* device_h_included */
