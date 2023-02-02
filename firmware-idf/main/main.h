#ifndef main_h_included
#define main_h_included

#define SNAPPY_LOGGING

/* Environment sensor: DFRobot SEN0500, hardwired to i2c 0x22
 * https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor
 */
#define SNAPPY_I2C_SEN0500
#ifdef SNAPPY_I2C_SEN0500
# define SEN0500_I2C_ADDRESS 0x22
#endif

/* Air/gas sensor: DFRobot SEN0514, hardwired to i2c 0x53
 * (Supposedly it can also be at 0x52)
 * https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor
 */
#define SNAPPY_I2C_SEN0514
#ifdef SNAPPY_I2C_SEN0514
# define SEN0514_I2C_ADDRESS 0x53
#endif

/* OLED: SSD1306-based 128x32 pixel display, hardwired to i2c 0x3C
 * Eg https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/
 */
#define SNAPPY_I2C_SSD1306
#ifdef SNAPPY_I2C_SSD1306
# define SSD1306_I2C_ADDRESS 0x3C
# define SSD1306_WIDTH 128
# define SSD1306_HEIGHT 32
# define SSD1306_INCLUDE_FONT_7x10 /* There are others, this is readable */
# define SSD1306_GRAPHICS
#endif

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

#endif /* !main_h_included */
