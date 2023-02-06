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

#include "esp32-hal-ledc.h"
#endif

#ifdef SNAPPY_HARDWARE_1_1_0
# define POWER_PIN 26		/* GPIO26 aka A0 aka DAC2: peripheral power */
# define BTN1_PIN 25		/* GPIO25 aka A1 aka DAC1: BTN1 aka WAKE */
# define PIR_PIN 34		/* GPIO34 aka A2: PIR */
# define I2C_BUS 0		/* Everything is on I2C bus 0 */
# define I2C_SCL_PIN 22		/* GPIO22: Standard I2C pin */
# define I2C_SDA_PIN 23		/* GPIO23: Standard I2C pin */
# define PWM_PIN T8	        /* Tentative: ADC1 CH5, pin IO33 aka pin T8 */
# define PWM_CHAN 5
# ifdef SNAPPY_I2C_SEN0500
#  define SEN0500_I2C_ADDRESS 0x22 /* Hardwired */
# endif
# ifdef SNAPPY_I2C_SEN0514
#  define SEN0514_I2C_ADDRESS 0x53 /* Hardwired, though 0x52 may be an option */
# endif
# ifdef SNAPPY_I2C_SSD1306
#  define SSD1306_I2C_ADDRESS 0x3C /* Hardwired */
# endif
#else
# error "Unsupported hardware generation"
#endif

#define PERIPHERAL_POWERUP_MS 1000 /* Wait time, maybe too much */
#define I2C_STABILIZE_MS      1000 /* Wait time, maybe too much */

#ifdef SNAPPY_I2C_SSD1306
/* On i2c1 */
uint8_t ssd1306_mem[SSD1306_DEVICE_SIZE(SSD1306_WIDTH, SSD1306_HEIGHT)];
SSD1306_Device_t* ssd1306; /* If non-null then we have a device */
#endif

#ifdef SNAPPY_I2C_SEN0514
/* On i2c1 */
bool have_sen0514;
dfrobot_sen0514_t sen0514; /* Only if have_sen0514 is true */
#endif

#ifdef SNAPPY_I2C_SEN0500
/* On i2c1 */
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

static QueueHandle_t evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t) arg;
  uint32_t ev = EV_NONE;
  switch (gpio_num) {
  case BTN1_PIN:
    ev = EV_BTN1 | (gpio_get_level(BTN1_PIN) << 4);
    break;
  case PIR_PIN:
    ev = EV_PIR | (gpio_get_level(PIR_PIN) << 4);
    break;
  }
  if (ev != EV_NONE) {
    xQueueSendFromISR(evt_queue, &ev, NULL);
  }
}

void install_interrupts(QueueHandle_t evt_queue_) {
  evt_queue = evt_queue_;
  gpio_install_isr_service(0);
}

void initialize_onboard_buttons() {
  gpio_config_t btn_conf = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .pin_bit_mask = (1ULL << BTN1_PIN),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&btn_conf);
  gpio_isr_handler_add(BTN1_PIN, gpio_isr_handler, (void*) BTN1_PIN);
}
  
#ifdef SNAPPY_I2C
void initialize_i2c() {
  /* Initialize I2C master */
  i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_SDA_PIN,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = I2C_SCL_PIN,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000, // 100KHz
  };
  if (i2c_param_config(I2C_BUS, &i2c_conf) != ESP_OK) {
    panic("failed to install i2c @ 1\n");
  }
  if (i2c_driver_install(I2C_BUS,
			 i2c_conf.mode,
			 /* rx_buf_ena= */ 0,
			 /* tx_buf_ena= */ 0,
			 /* intr_flags= */ 0) != ESP_OK) {
    panic("failed to install i2c @ 2\n");
  }
  i2c_set_timeout((i2c_port_t)I2C_BUS, 0xFFFFF);

  /* Some of the i2c devices are slow to come up. */
  vTaskDelay(pdMS_TO_TICKS(I2C_STABILIZE_MS));
}
#endif

#ifdef SNAPPY_I2C_SEN0500
void initialize_i2c_sen0500() {
  /* Check the environment sensor and initialize it */
  if (!(have_sen0500 = dfrobot_sen0500_begin(&sen0500, I2C_BUS, SEN0500_I2C_ADDRESS))) {
    LOG("Failed to init SEN0500\n");
  }
}
#endif
  
#ifdef SNAPPY_I2C_SEN0514
void initialize_i2c_sen0514() {
  if (!(have_sen0514 = dfrobot_sen0514_begin(&sen0514, I2C_BUS, SEN0514_I2C_ADDRESS))) {
    LOG("Failed to init SEN0514");
  }
}
#endif

#ifdef SNAPPY_I2C_SSD1306
void initialize_i2c_ssd1306() {
  /* Check the OLED and initialize it */
  ssd1306 = ssd1306_Create(ssd1306_mem, I2C_BUS, SSD1306_I2C_ADDRESS, SSD1306_WIDTH, SSD1306_HEIGHT);
  if (!ssd1306) {
    LOG("Failed to init SSD1306");
  }
}

bool ssd1306_Write_Blocking(unsigned i2c_num, unsigned device_address, unsigned mem_address,
			    uint8_t* write_buffer, size_t write_size) {
  /* Based on i2c_master_write_to_device() */
  /*
   * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
   *
   * SPDX-License-Identifier: Apache-2.0
   */

  esp_err_t err = ESP_OK;
  uint8_t buffer[I2C_LINK_RECOMMENDED_SIZE(2)] = { 0 };

  i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
  assert (handle != NULL);

  err = i2c_master_start(handle);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 1\n");
    goto end;
  }

  err = i2c_master_write_byte(handle, device_address << 1 | I2C_MASTER_WRITE, true);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 2\n");
    goto end;
  }

  err = i2c_master_write_byte(handle, mem_address, true);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 3\n");
    goto end;
  }
  
  err = i2c_master_write(handle, write_buffer, write_size, true);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 4\n");
    goto end;
  }

  i2c_master_stop(handle);
  err = i2c_master_cmd_begin(i2c_num, handle, portMAX_DELAY);
  if (err != ESP_OK) {
    LOG("Fail OLED @ 5\n");
  }

 end:
  i2c_cmd_link_delete_static(handle);
  return err == ESP_OK;
}
#endif

#ifdef SNAPPY_GPIO_SEN0171
void initialize_gpio_sen0171() {
  gpio_config_t pir_conf = {
    .intr_type = GPIO_INTR_POSEDGE,
    .pin_bit_mask = (1ULL << PIR_PIN),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&pir_conf);
  /* Handler support has been installed by general GPIO initialization */
}

void enable_gpio_sen0171() {
  gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);

  /* If the device has already detected motion we won't get an
     interrupt, so check specially for this. */
  if (gpio_get_level(PIR_PIN)) {
    uint32_t ev = EV_PIR | 1 << 4;
    xQueueSend(evt_queue, &ev, portMAX_DELAY);
  }
}

void disable_gpio_sen0171() {
  gpio_isr_handler_remove(PIR_PIN);
}
#endif

#ifdef SNAPPY_GPIO_PIEZO
void setup_sound() {
  // TODO: Justify these choices, which are wrong.  Probably 1 bit of resolution is fine?
  // And the question is whether ledcSetup should be before or after ledcAttachPin.
  //ledcSetup(PWM_CHAN, 4000, 16);
  ledcAttachPin(PWM_PIN, PWM_CHAN);
}

static void start_note(int frequency) {
  ledcWriteTone(PWM_CHAN, frequency);
}

static void stop_note() {
  ledcWrite(PWM_CHAN, 0);
}
#endif
