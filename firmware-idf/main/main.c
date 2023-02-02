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

#include "main.h"
#include "snappy_i2c.h"
#include "dfrobot_sen0500.h"
#include "ssd1306.h"

/* SnappySense hardware version.  Some pins changed from v1.0.0 to v1.1.0 */
#define HARDWARE_1_1_0

#ifdef HARDWARE_1_1_0
/* GPIO26 aka A0 aka DAC2: peripheral power */
#  define POWER_PIN 26

/* GPIO25 aka pin A1 aka DAC1: the external wake button on the board */
#  define BUTTON_PIN 25

/* GPIO34 aka A2: PIR */
#  define PIR_PIN 34

/* I2C1: GPIO22 and GPIO23 are the standard I2C pins */
#define I2C_SCL_PIN 22
#define I2C_SDA_PIN 23

#else
#  error "Fix your definitions"
#endif

#ifdef SNAPPY_I2C_SSD1306
/* On i2c1 */
static uint8_t ssd1306_mem[SSD1306_DEVICE_SIZE(SSD1306_WIDTH, SSD1306_HEIGHT)];
static SSD1306_Device_t* ssd1306; /* If non-null then we have a device */
#endif

#ifdef SNAPPY_I2C_SEN0514
/* On i2c1 */
static bool have_sen0514;
#endif

#ifdef SNAPPY_I2C_SEN0500
/* On i2c1 */
static bool have_sen0500;
static dfrobot_sen0500_t sen0500; /* Only if have_sen0500 is true */
#endif

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

  /* Give peripheral power the time to stabilize.  */
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  /* Enable interrupts.  They will be communicated on the queue from the ISR. */
#if 0
  gpio_install_isr_service(0);
#endif
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  /* Setup a PIR interrupt.  Note this is mostly an experiment, making the PIR interrupt-driven
   * conflicts with a hardware bug on the ESP32, if we also want wifi.
   */
#if 0
  gpio_config_t pir_conf = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .pin_bit_mask = (1ULL << PIR_PIN) | (1ULL << BUTTON_PIN),
    .mode = GPIO_MODE_INPUT,
  };
  gpio_config(&pir_conf);
  gpio_isr_handler_add(PIR_PIN, gpio_isr_handler, (void*) PIR_PIN);
  gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*) BUTTON_PIN);
#endif

#ifdef SNAPPY_I2C
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
  i2c_set_timeout((i2c_port_t)i2c_master_port, 0xFFFFF);

  /* Some of the i2c devices are slow to come up.  200ms may be more than we need. */
  vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif

#ifdef SNAPPY_I2C_SEN0500
  /* Check the DHT device and initialize it */
  if (dfrobot_sen0500_begin(&sen0500, i2c_master_port, SEN0500_I2C_ADDRESS)) {
    have_sen0500 = true;
  } else {
    LOG("Failed to init dht\n");
  }
#endif
  
#ifdef SNAPPY_I2C_SSD1306
  ssd1306 = ssd1306_i2c_init(ssd1306_mem, 0, SSD1306_I2C_ADDRESS, SSD1306_WIDTH, SSD1306_HEIGHT);
  if (ssd1306) {
    ssd1306_Init(ssd1306);
  }
#endif

  /* Create a clock tick used to drive device readings */
  TimerHandle_t t = xTimerCreate("sensor-clock", pdMS_TO_TICKS(5*1000), pdTRUE, NULL, clock_callback);
  xTimerStart(t, 0);

  /* And we are up! */
  LOG("Snappysense running!\n");
#ifdef SNAPPY_I2C_SSD1306
  if (ssd1306) {
    ssd1306_SetCursor(ssd1306, 0, 0);
    ssd1306_WriteString(ssd1306, "Hello, world!", Font_7x10, SSD1306_WHITE);
    ssd1306_UpdateScreen(ssd1306);
  }
#endif

  /* Process events forever */
  struct timeval button_down; /* Time of button press */
  bool was_pressed = false;   /*   if this is true */
  for(;;) {
    uint32_t ev;
    if(xQueueReceive(gpio_evt_queue, &ev, portMAX_DELAY)) {
      switch (ev & 15) {
      case EV_PIR:
	LOG("PIR: %" PRIu32 "\n", ev >> 4);
	break;
      case EV_BUTTON: {
	/* Experimentation suggests that it's possible to have spurious button presses of around 10K
	   us, when the finger nail sort of touches the edge of the button and slides off.  A "real"
	   press lasts at least 100K us. */
	uint32_t state = ev >> 4;
	LOG("BUTTON: %" PRIu32 "\n", state);
	if (!state && was_pressed) {
	  struct timeval now;
	  gettimeofday(&now, NULL);
	  uint64_t t = ((uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_usec) -
	    ((uint64_t)button_down.tv_sec * 1000000 + (uint64_t)button_down.tv_usec);
	  LOG("  Pressed for %" PRIu64 "us\n", t);
	}
	was_pressed = state == 1;
	if (state) {
	  gettimeofday(&button_down, NULL);
	}
	break;
      }
      case EV_CLOCK: {
	LOG("CLOCK\n");
	float temp = 0.0f;
#ifdef SNAPPY_I2C_SEN0500
	if (!have_sen0500) {
	  LOG("  No temperature sensor\n");
	} else if (dfrobot_sen0500_get_temperature(&sen0500, DFROBOT_SEN0500_TEMP_C, &temp)) {
# ifdef SNAPPY_I2C_SSD1306
	  if (ssd1306) {
	    ssd1306_Fill(ssd1306, SSD1306_BLACK);
	    ssd1306_SetCursor(ssd1306, 0, 0);
	    char buf[32];
	    LOG(buf, "Temperature: %.1f", temp);
	    ssd1306_WriteString(ssd1306, buf, Font_7x10, SSD1306_WHITE);
	    ssd1306_UpdateScreen(ssd1306);
	  } else {
	    LOG("  Temperature = %f\n", temp);
	  }
#else
	  LOG("  Temperature = %f\n", temp);
# endif
	} else {
	  LOG("  Failed to read temperature.\n");
	}
#endif
	break;
      }
      default:
	LOG("Unknown event: %" PRIu32 "\n", ev);
	break;
      }
    }
  }
}

void panic(const char* msg) {
  LOG("PANIC: %s\n", msg);
  for(;;) {}
}

#ifdef SNAPPY_I2C_SSD1306

void ssd1306_Delay(unsigned ms) {
  vTaskDelay(ms / portTICK_PERIOD_MS);
}

void ssd1306_Write_Blocking(unsigned i2c_num, unsigned device_address, unsigned mem_address,
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
}

#endif
