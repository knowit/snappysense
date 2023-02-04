#ifndef device_h_included
#define device_h_included

#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void power_up_peripherals();
void install_interrupts(QueueHandle_t gpio_evt_queue);

#ifdef SNAPPY_GPIO_SEN0171
void initialize_gpio_sen0171();
#endif

#ifdef SNAPPY_I2C
void initialize_i2c();
#endif

#ifdef SNAPPY_I2C_SEN0500
void initialize_i2c_sen0500();
#endif
/* Returns true iff a temperature sensor was present and could be
 * read; the temperature may however not be sensible during warmup.
 */
bool read_temperature(float* temperature);

#ifdef SNAPPY_GPIO_SEN0171
void initialize_gpio_sen0171();
#endif

#ifdef SNAPPY_I2C_SSD1306
void initialize_i2c_ssd1306();
#endif
void show_text(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

#ifdef SNAPPY_GPIO_PIEZO
void setup_sound();
void start_note(int frequency);
void stop_note();
#endif

#endif /* device_h_included */
