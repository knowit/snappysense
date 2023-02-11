/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* The device layer is strictly about device access and control.  Any higher-level work, such as
   creating separate tasks to perform sampling or sound output, is handled in the caller.

   The more complex devices have driver files of their own; simpler devices are handled directly in
   this file.  */

#include "device.h"

#include <stdarg.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#ifdef SNAPPY_I2C_SEN0500
# include "dfrobot_sen0500.h"
#endif
#ifdef SNAPPY_I2C_SEN0514
# include "dfrobot_sen0514.h"
#endif
#ifdef SNAPPY_I2C_SSD1306
# include "ssd1306.h"
#endif

#ifdef SNAPPY_GPIO_PIEZO
/* FIXME: This is arduino stuff.  Instead, we want to include "driver/ledc.h" and we want to control
   the pulse in the same way that the arduino library does.  Some inspiration for this may be taken
   from the way Gunnar did it in his prototype. */

//#include "esp32-hal-ledc.h"
#endif

#ifdef SNAPPY_ADC_SEN0487
/* FIXME: Include something from the ADC subsystem */
/* This looks like an ADC oneshot mode situation */
#include "esp_adc/adc_oneshot.h"
/* Whether calibration is needed is TBD.  See app example */
#endif

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

void power_up_peripherals() {
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

static void IRAM_ATTR gpio_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t) arg;
  snappy_event_t ev = EV_NONE;
  switch (gpio_num) {
  case BTN1_PIN:
    ev = EV_BTN1 | (gpio_get_level(BTN1_PIN) << 4);
    break;
  case PIR_PIN:
    ev = EV_MOTION;
    break;
  }
  if (ev != EV_NONE) {
    xQueueSendFromISR(snappy_event_queue, &ev, NULL);
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
bool initialize_i2c() {
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
					SSD1306_WIDTH, SSD1306_HEIGHT));
}

bool ssd1306_WriteI2C(unsigned i2c_num, unsigned device_address, unsigned mem_address,
		      uint8_t* write_buffer, size_t write_size) {
  /* Based on i2c_master_write_to_device() */

  /* SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
     SPDX-License-Identifier: Apache-2.0 */

  esp_err_t err = ESP_OK;
  uint8_t buffer[I2C_LINK_RECOMMENDED_SIZE(2)] = { 0 };

  i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
  assert (handle != NULL);

  err = i2c_master_start(handle);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 1");
    goto end;
  }

  err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_WRITE, true);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 2");
    goto end;
  }

  err = i2c_master_write_byte(handle, mem_address, true);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 3");
    goto end;
  }
  
  err = i2c_master_write(handle, write_buffer, write_size, true);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 4");
    goto end;
  }

  i2c_master_stop(handle);
  err = i2c_master_cmd_begin(i2c_num, handle, portMAX_DELAY);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 5");
  }

 end:
  i2c_cmd_link_delete_static(handle);
  return err == ESP_OK;
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
    snappy_event_t ev = EV_MOTION;
    xQueueSend(snappy_event_queue, &ev, portMAX_DELAY);
  }
}

void disable_gpio_sen0171() {
  gpio_isr_handler_remove(PIR_PIN);
}
#endif

#ifdef SNAPPY_GPIO_PIEZO
bool initialize_gpio_piezo() {
  // TODO: Justify these choices, which are wrong.  Probably 1 bit of resolution is fine?
  // And the question is whether ledcSetup should be before or after ledcAttachPin.
  //ledcSetup(PWM_CHAN, 4000, 16);
  //  ledcAttachPin(PWM_PIN, PWM_CHAN);
  return true;
}

void start_note(int frequency) {
  //  ledcWriteTone(PWM_CHAN, frequency);
}

void stop_note() {
  //  ledcWrite(PWM_CHAN, 0);
}
#endif

#ifdef SNAPPY_ADC_SEN0487
bool initialize_adc_sen0487() {
  /* FIXME: Almost certainly something to do */
  return true;
}

unsigned sen0487_sound_level() {
  /* FIXME: analogRead is Arduino */
  //  unsigned r = analogRead(MIC_PIN);
  unsigned r = 0;
  /* TODO - This scale is pretty arbitrary */
  if (r < 1700) {
    return 1;
  }
  if (r < 2000) {
    return 2;
  }
  if (r < 2500) {
    return 3;
  }
  if (r < 3000) {
    return 4;
  }
  return 5;
}
#endif
