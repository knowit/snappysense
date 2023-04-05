/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* The device layer is strictly about device access and control.  Any higher-level work, such as
   creating separate tasks to perform sampling or sound output, is handled in the caller.

   This file is mainly about hiding some information from the rest of the system: pin numbers, bus
   numbers, and so on.  Most devices have driver files of their own that contain further
   initialization code, normally called from within the present file.  */

#include "device.h"

#include <stdarg.h>
#include "driver/gpio.h"
#include "driver/i2c.h"

#include "dfrobot_sen0487.h"
#include "dfrobot_sen0500.h"
#include "dfrobot_sen0514.h"
#include "ssd1306.h"
#include "esp32_ledc_piezo.h"

#ifdef SNAPPY_HARDWARE_1_1_0
# define POWER_PIN 26		/* GPIO26 aka A0 aka DAC2: peripheral power */
# define BTN1_PIN 25		/* GPIO25 aka A1 aka DAC1: BTN1 aka WAKE */
# define PIR_PIN 34		/* GPIO34 aka A2: PIR */
# define MIC_PIN 4		/* GPIO4 aka A5: MEMS ADC */
# define I2C1_BUS 0		/* Everything is on I2C bus 0 */
# define I2C_SCL_PIN 22		/* GPIO22: Standard I2C pin */
# define I2C_SDA_PIN 23		/* GPIO23: Standard I2C pin */
# define PWM_PIN T8	        /* Tentative: ADC1 CH5, pin IO33 aka pin T8 */
# define PWM_CHAN 5
#else
# error "Unsupported hardware generation"
#endif

#define PERIPHERAL_POWERUP_MS 1000 /* Wait time, maybe too much */
#define I2C_STABILIZE_MS      1000 /* Wait time, maybe too much */

#ifdef SNAPPY_I2C_SSD1306
# define SSD1306_BUS I2C1_BUS
# define SSD1306_ADDRESS 0x3C /* Hardwired */
bool have_ssd1306;
SSD1306_Device_t ssd1306; /* If non-null then we have a device */
#endif

#ifdef SNAPPY_I2C_SEN0514
# define SEN0514_BUS I2C1_BUS
# define SEN0514_ADDRESS 0x53 /* Hardwired, though 0x52 may be an option */
bool have_sen0514;
dfrobot_sen0514_t sen0514; /* Only if have_sen0514 is true */
#endif

#ifdef SNAPPY_I2C_SEN0500
# define SEN0500_BUS I2C1_BUS
# define SEN0500_ADDRESS 0x22 /* Hardwired */
bool have_sen0500;
dfrobot_sen0500_t sen0500; /* Only if have_sen0500 is true */
#endif

static bool regulator_on;

void enable_regulator() {
  if (!regulator_on) {
    /* Enable peripheral power */
    gpio_config_t power_conf = {
      .pin_bit_mask = (1ULL << POWER_PIN),
      .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&power_conf);
    gpio_set_level(POWER_PIN, 1);

    /* Give peripheral power the time to stabilize.  */
    vTaskDelay(pdMS_TO_TICKS(PERIPHERAL_POWERUP_MS));
  }
}

void disable_regulator() {
  /* This is unconditional as a failsafe */
  gpio_set_level(POWER_PIN, 0);
  regulator_on = false;
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t) arg;
  switch (gpio_num) {
  case BTN1_PIN:
    put_main_event_from_isr(gpio_get_level(BTN1_PIN) ? EV_BUTTON_DOWN : EV_BUTTON_UP);
    break;
  case PIR_PIN:
    put_main_event_from_isr(EV_MOTION_DETECTED);
    break;
  }
}

void install_interrupts() {
  gpio_install_isr_service(0);
}

void initialize_onboard_buttons() {
  gpio_config_t btn_conf = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .pin_bit_mask = (1ULL << BTN1_PIN),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&btn_conf);
}
  
void enable_onboard_buttons() {
  gpio_isr_handler_add(BTN1_PIN, gpio_isr_handler, (void*) BTN1_PIN);
}

bool btn1_is_pressed() {
  return gpio_get_level(BTN1_PIN);
}

#ifdef SNAPPY_I2C
bool enable_i2c() {
#if defined(I2C2_BUS) || defined(I2C3_BUS)
# error "More code needed"
#endif

  /* Initialize ourselves as master on I2C1 */
  i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_SDA_PIN,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = I2C_SCL_PIN,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000, // 100KHz
  };
  if (i2c_param_config(I2C1_BUS, &i2c_conf) != ESP_OK) {
    return false;
  }
  if (i2c_driver_install(I2C1_BUS,
			 i2c_conf.mode,
			 /* rx_buf_ena= */ 0,
			 /* tx_buf_ena= */ 0,
			 /* intr_flags= */ 0) != ESP_OK) {
    return false;
  }
  i2c_set_timeout((i2c_port_t)I2C1_BUS, 0xFFFFF);

  /* Some of the i2c devices are slow to come up. */
  vTaskDelay(pdMS_TO_TICKS(I2C_STABILIZE_MS));
  return true;
}

bool disable_i2c() {
  /* Remove the driver. */
  i2c_driver_delete(I2C1_BUS);

  /* Pull the I2C signals down.  This is different from just disabling pullup and setting the
     signals to zero; the AIR sensor will not be properly reset unless we pull them down. */
  gpio_pullup_dis((gpio_num_t)I2C_SDA_PIN);
  gpio_pulldown_en((gpio_num_t)I2C_SDA_PIN);
  gpio_pullup_dis((gpio_num_t)I2C_SCL_PIN);
  gpio_pulldown_en((gpio_num_t)I2C_SCL_PIN);

  /* FIXME? */
  return true;
}
#endif

#ifdef SNAPPY_I2C_SEN0500
bool initialize_i2c_sen0500() {
  /* Check the environment sensor and initialize it */
  return (have_sen0500 = dfrobot_sen0500_begin(&sen0500, SEN0500_BUS, SEN0500_ADDRESS));
}
#endif
  
#ifdef SNAPPY_I2C_SEN0514
bool initialize_i2c_sen0514() {
  return (have_sen0514 = dfrobot_sen0514_begin(&sen0514, SEN0514_BUS, SEN0514_ADDRESS));
}
#endif

#ifdef SNAPPY_I2C_SSD1306
bool initialize_i2c_ssd1306() {
  /* Check the OLED and initialize it */
  return (have_ssd1306 = ssd1306_Create(&ssd1306, SSD1306_BUS, SSD1306_ADDRESS,
					SSD1306_WIDTH, SSD1306_HEIGHT, /* flags= */ 0));
}
#endif

#ifdef SNAPPY_GPIO_SEN0171
bool initialize_gpio_sen0171() {
  gpio_config_t pir_conf = {
    .intr_type = GPIO_INTR_POSEDGE,
    .pin_bit_mask = (1ULL << PIR_PIN),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&pir_conf);
  /* Handler support has been installed by general GPIO initialization */
  return true;
}

void enable_gpio_sen0171() {
  gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);

  /* If the device has already detected motion we won't get an interrupt; handle this. */
  if (gpio_get_level(PIR_PIN)) {
    put_main_event(EV_MOTION_DETECTED);
  }
}

void disable_gpio_sen0171() {
  gpio_isr_handler_remove(PIR_PIN);
}
#endif

#ifdef SNAPPY_ESP32_LEDC_PIEZO
bool initialize_esp32_ledc_piezo() {
  /* TODO: Maybe this needs to setup some pins at least. */
  return piezo_begin();
}
#endif

#ifdef SNAPPY_ADC_SEN0487
bool initialize_adc_sen0487() {
  /* FIXME: Almost certainly something to do */
  return true;
}
#endif
