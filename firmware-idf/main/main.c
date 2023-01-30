/* SnappySense driver code based on ESP32-IDF, no Arduino */

/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "snappy_i2c.h"
#include "dfrobot_sen0500.h"

/* SnappySense hardware version.  Some pins changed from v1.0.0 to v1.1.0 */
#define HARDWARE_1_1_0

#ifdef HARDWARE_1_1_0
/* GPIO26 aka A0 aka DAC2: peripheral power */
#  define POWER_PIN 26

/* GPIO25 aka pin A1 aka DAC1: the external wake button on the board */
#  define BUTTON_PIN 25

/* GPIO34 aka A2: PIR */
#  define PIR_PIN 34

/* GPIO22 and GPIO23 are the standard I2C pins */
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 23

/* I2C devices we know about, see snappysense sources for more info */
#define I2C_OLED_ADDRESS 0x3C
#define I2C_AIR_ADDRESS  0x53
#define I2C_DHT_ADDRESS  0x22

#else
#  error "Fix your definitions"
#endif

#ifdef I2C_OLED_ADDRESS
static bool have_oled = false;
#endif
#ifdef I2C_AIR_ADDRESS
static bool have_air = false;
#endif
#ifdef I2C_DHT_ADDRESS
static bool have_dht = false;
static dfrobot_sen0500_t dht;
#endif

/* Signal error and hang */
void panic(const char* msg) __attribute__ ((noreturn));

/* Event numbers should stay below 16.  Events have an event number in the
 * low 4 bits and payload in the upper 28. */
enum {
  EV_NONE,
  EV_PIR,			/* Payload: pin level, 0 (no motion) or 1 (motion) */
  EV_BUTTON,			/* Payload: button level, 0 (up) or 1 (down) */
  EV_CLOCK,			/* Payload: nothing */
};

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t) arg;
  uint32_t ev = EV_NONE;
  switch (gpio_num) {
  case PIR_PIN:
    ev = EV_PIR | (gpio_get_level(PIR_PIN) << 4);
    break;
  case BUTTON_PIN:
    ev = EV_BUTTON | (gpio_get_level(BUTTON_PIN) << 4);
    break;
  }
  if (ev != EV_NONE) {
    xQueueSendFromISR(gpio_evt_queue, &ev, NULL);
  }
}

static void clock_callback(TimerHandle_t t) {
  uint32_t ev = EV_CLOCK;
  xQueueSend(gpio_evt_queue, &ev, 0);
}

void app_main(void)
{
  /* Enable peripheral power */
  gpio_config_t power_conf = {
    .pin_bit_mask = (1ULL << POWER_PIN),
    .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&power_conf);
  gpio_set_level(POWER_PIN, 1);

  /* Enable interrupts.  They will be communicated on the queue from the ISR. */
  gpio_install_isr_service(0);
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  /* Setup a PIR interrupt.  Note this is mostly an experiment, making the PIR interrupt-driven
   * conflicts with a hardware bug on the ESP32, if we also want wifi.
   */
  gpio_config_t pir_conf = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .pin_bit_mask = (1ULL << PIR_PIN) | (1ULL << BUTTON_PIN),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&pir_conf);
  gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
  gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*) BUTTON_PIN);

  /* Initialize I2C master */
  int i2c_master_port = 0; /* I2C bus #1 */
  i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_SDA_PIN,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = I2C_SCL_PIN,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000, // 100KHz
  };
  if (i2c_param_config(i2c_master_port, &i2c_conf) != ESP_OK) {
    panic("failed to install i2c @ 1\n");
  }
  if (i2c_driver_install(i2c_master_port,
			 i2c_conf.mode,
			 /* rx_buf_ena= */ 0,
			 /* tx_buf_ena= */ 0,
			 /* intr_flags= */ 0) != ESP_OK) {
    panic("failed to install i2c @ 2\n");
  }
  /* Some of the i2c devices are slow to come up.  200ms may be more than we need. */
  vTaskDelay(200);
 
  /* Look for i2c devices. Technically this is probably not necessary, as the devices
   * are being probed further below.
   */
  for ( int address=1 ; address < 127; address++ ) {
    if (snappy_i2c_probe(i2c_master_port, address)) {
#ifdef I2C_OLED_ADDRESS
      have_oled = have_oled || (address == I2C_OLED_ADDRESS);
#endif
#ifdef I2C_AIR_ADDRESS
      have_air = have_air || (address == I2C_AIR_ADDRESS);
#endif
#ifdef I2C_DHT_ADDRESS
      have_dht = have_dht || (address == I2C_DHT_ADDRESS);
#endif
    }
  }

#ifdef I2C_DHT_ADDRESS
  /* Check the DHT device and initialize it */
  if (have_dht) {
    if (!dfrobot_sen0500_begin(&dht, i2c_master_port, I2C_DHT_ADDRESS)) {
      have_dht = false;
    }
  }
#endif

  /* Create a clock tick used to drive device readings */
  TimerHandle_t t = xTimerCreate("sensor-clock", pdMS_TO_TICKS(5*1000), pdTRUE, NULL, clock_callback);
  xTimerStart(t, 0);

  /* And we are up! */
  printf("Snappysense running!\n");

  /* Process events forever */
  struct timeval button_down; /* Time of button press */
  bool was_pressed = false;   /*   if this is true */
  for(;;) {
    uint32_t ev;
    if(xQueueReceive(gpio_evt_queue, &ev, portMAX_DELAY)) {
      switch (ev & 15) {
      case EV_PIR:
	printf("PIR: %" PRIu32 "\n", ev >> 4);
	break;
      case EV_BUTTON: {
	/* Experimentation suggests that it's possible to have spurious button presses of around 10K
	   us, when the finger nail sort of touches the edge of the button and slides off.  A "real"
	   press lasts at least 100K us. */
	uint32_t state = ev >> 4;
	printf("BUTTON: %" PRIu32 "\n", state);
	if (!state && was_pressed) {
	  struct timeval now;
	  gettimeofday(&now, NULL);
	  uint64_t t = ((uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_usec) -
	    ((uint64_t)button_down.tv_sec * 1000000 + (uint64_t)button_down.tv_usec);
	  printf("  Pressed for %" PRIu64 "us\n", t);
	}
	was_pressed = state == 1;
	if (state) {
	  gettimeofday(&button_down, NULL);
	}
	break;
      }
      case EV_CLOCK: {
	float temp = 0.0f;
	if (dfrobot_sen0500_get_temperature(&dht, DFROBOT_SEN0500_TEMP_C, &temp)) {
	  printf("CLOCK.  Temp = %f\n", temp);
	} else {
	  printf("CLOCK.  Failed to read temperature.\n");
	}
	break;
      }
      default:
	printf("Unknown event: %" PRIu32 "\n", ev);
	break;
      }
    }
  }
}

void panic(const char* msg) {
  puts("PANIC\n");
  puts(msg);
  for(;;) {}
}
