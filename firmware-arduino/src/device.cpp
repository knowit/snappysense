// Interface to SnappySense hardware device (see the hardware/ subdirectory)

// Hardware v1.0.0 and v1.1.0:
//
// Adafruit feather esp32 "HUZZAH32" (https://www.adafruit.com/product/3406)
//   esp32-wroom-32 MoC, https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf
//   onboard wifi for communication
//   onboard usb serial for power and development
//   onboard reset button
//   onboard configured i2c for communication with peripherals
//   onboard connectivity to external rechargeable LiPoly battery, charging from onboard usb
//
// Integrated button connected to pin A1 of the esp32
//
// 0.91" 128x32 OLED display @ I2C 0x3C (hardwired)
//   eg https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/
//
//   TODO: Document API?
//
// DFRobot SKU:SEN0514 air/gas sensor @ I2C 0x53 (alternate 0x52, supposedly)
//   https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor
//
//   TODO: Document API?
//
// DFRobot SKU:SEN0500 environmental sensor @ I2C 0x22 (undocumented)
//   https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor
//
//   TODO: Document API?
//
// DFRobot SKU:SEN0171 Digital passive IR sensor @ pin (see below), signal reads analog
//   high after / while movement is detected, low when no movement is detected, no buffering
//   of value, see issue #9.  Reading it digital would be fine too.
//   https://www.dfrobot.com/product-1140.html
//   https://wiki.dfrobot.com/PIR_Motion_Sensor_V1.0_SKU_SEN0171
//
//   HW 1.1.0: The PIR is connected to pin A2.
//
//   HW 1.0.0: The PIR is connected to pin A4.
//
// DFRobot SKU:SEN0487 MEMS microphone, analog signal (see below), unit and range of signal
//   not documented beyond analogRead() returning an uint16_t.  From testing, it looks
//   like it returns a reading mostly in the range 1500-2500, corresponding to 1.5V-2.5V,
//   presumably the actual upper limit is around 3V somewhere.
//   https://www.dfrobot.com/product-2357.html
//   https://wiki.dfrobot.com/Fermion_MEMS_Microphone_Sensor_SKU_SEN0487
//
//   TODO: Document the use of the specific ADC on the ESP32 for this.
//
//   HW 1.1.0: The MEMS is connected to pin A3.
//
//   HW 1.0.0: The MEMS is connected to pin A5, but this is a bug because the ADC2 configured
//   for the MEMS conflicts with its hardwired use for WiFi, as a consequence, they can't be
//   used at the same time.  In practice the Mic functionality is unavailable in this rev.
//
// Hardware v1.1.0:
//
// TODO: Piezo element
//   The PWM used to drive the element is here:
//   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html

#include "device.h"

#include "config.h"
#include "icons.h"
#include "log.h"
#include "sensor.h"
#include "time_server.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobot_ENS160.h>
#include <DFRobot_EnvironmentalSensor.h>
#include <esp32-hal-ledc.h>
#include <Wire.h>

// SnappySense 1.x.y device definition

// Pin definitions
#define POWER_ENABLE_PIN A0
#ifdef SNAPPY_BUTTON
# define BUTTON_PIN A1  // WAKE on 1.0.0, BTN1 on 1.1.0
#endif
#if defined(SNAPPY_HARDWARE_1_0_0)
# define PIR_SENSOR_PIN A4
# define MIC_PIN A5
#elif defined(SNAPPY_HARDWARE_1_1_0)
# define PIR_SENSOR_PIN A2
# define MIC_PIN A3
#else
# error "Fix your hardware definitions"
#endif
#define I2C_MASTER_SDA SDA
#define I2C_MASTER_SCL SCL
#define PWM_PIN T8  // Tentative: ADC1 CH5, pin IO33 aka pin T8
#define PWM_CHAN 5
#ifdef SNAPPY_I2CCONFIG
// I2C #1 pins.  On the HUZZAH32 module, these two should be directly above the I2C #0 pins,
// and in the same order vertically: if we think of the the WiFi antenna as pointing "down"
// then the slave SDA is below the slave SCL.
# define I2C_SLAVE_SDA 14
# define I2C_SLAVE_SCL 32
#endif

// I2C addresses
#define I2C_OLED_ADDRESS 0x3C
#define I2C_AIR_ADDRESS  0x53
#define I2C_DHT_ADDRESS  0x22
#ifdef SNAPPY_I2CCONFIG
# define I2C_SLAVE_ADDRESS 0x28
#endif

// OLED Display Constants
#define ICON_TEXT_START_X 48
#define ICON_TEXT_START_Y 8
#define ICON_HEIGHT   32
#define ICON_WIDTH    32
#define LOGO_HEIGHT   29
#define LOGO_WIDTH    128

static Adafruit_SSD1306 display(128, 32, &Wire);
static DFRobot_ENS160_I2C ENS160(&Wire, I2C_AIR_ADDRESS);
static DFRobot_EnvironmentalSensor environment(I2C_DHT_ADDRESS, /*pWire = */&Wire);
static bool peripherals_powered_on = false;
static bool have_environment;
static bool have_air;
static bool air_is_primed;
static unsigned sequence_number;
static unsigned pir_value;
static unsigned mems_value;

#ifdef SNAPPY_BUTTON
static void button_handler() {
  put_main_event_from_isr(digitalRead(BUTTON_PIN) ? EvCode::BUTTON_DOWN : EvCode::BUTTON_UP);
}
#endif

// This must NOT depend on the configuration because the configuration may not
// have been read at this point, see main.cpp.

void device_setup() {
#if defined(LOGGING) || defined(SNAPPY_SERIAL_INPUT)
  Serial.begin(115200);
#endif
#ifdef LOGGING
  // Connect the serial port whether anyone's listening or not.  This is in favor
  // of being able to plug in a serial cable to check on what's going on, after startup.
  set_log_stream(&Serial);
#endif

  // Set up io pins
  pinMode(POWER_ENABLE_PIN, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);
#ifdef SNAPPY_BUTTON
  pinMode(BUTTON_PIN, INPUT);
#endif

  // Bring up all peripherals
  power_peripherals_on();

  log("Device initialized\n");

#ifdef SNAPPY_BUTTON
  attachInterrupt(BUTTON_PIN, button_handler, CHANGE);
#endif
}

void power_peripherals_on() {
  if (!peripherals_powered_on) {
    // Initialize state variables
    have_air = false;
    have_environment = false;
    air_is_primed = false;
    reset_pir_and_mems();

    // Turn on peripheral power, must be on for i2c to work!
    digitalWrite(POWER_ENABLE_PIN, HIGH);
    // Wait until peripherals are stable.  100ms is not enough (issue #14), and 1000ms does not
    // really seem to be a hardship, so why not.
    delay(1000);

    // Init i2c
    // Note the (int) cast, some versions of the ESP32 libs need this, ref
    // https://github.com/espressif/arduino-esp32/issues/6616#issuecomment-1184167285
    Wire.begin((int) I2C_MASTER_SDA, I2C_MASTER_SCL);

    have_environment = environment.begin() == 0;
    have_air = ENS160.begin() == 0;

    // init oled display
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    display.begin(SSD1306_SWITCHCAPVCC, I2C_OLED_ADDRESS);
    peripherals_powered_on = true;
  }
}

#ifdef SNAPPY_I2CCONFIG
// The I2C slave is special purpose, used only when the device is in config mode and is
// listening for configurations on I2C.  Before calling this the device must be fully
// powered up.
//
// The slave interface uses Wire1 and we can assume this globally.
//
// There is no power-off functionality for this because config mode ends by the device
// being reset or powered off.
//
// RPi                          Huzzah32
// ---------------------        ---------
// GPIO 2 / Pin 3 / SDA1  <-->  IO14 / T6
// GPIO 3 / Pin 5 / SDCL  <-->  IO32 / T9
// GND    / Pin 6         <-->  GND

void power_i2c_slave_on() {
  assert(peripherals_powered_on);
  log("I2C slave initializing\n");

  // The i2c subsystem performs the necessary configuration of pins as part of begin()

  // The logic for TwoWire is that input arrives in packets.  Each packet is placed at
  // the beginning of the buffer, displacing what was already there.  The onReceive
  // callback is invoked.  Then we can read individual bytes.  Meanwhile, data may keep
  // arriving.  It is placed in a ring buffer at the IDF level.   There is a real risk
  // that data will wrap around and overwrite what's at the beginning if we keep
  // pumping it.
  //
  // In practice, we need flow control.  We need to know what a reasonable packet size
  // is, and how to wait to send more data.  Note that the synchronization may write I2C
  // data too.
  //
  // The simplest low-level protocol is for there to be an address that can be read to
  // determine whether we're CTS, and if so, we send + delay a little bit, and then check
  // the flag again and wait/delay.  "Reading an address" is a kind of request, which
  // is basically a write + read pair from the master.  It is important that we don't
  // flood the channel with RTS packets.  It may be that the best CTS is to delay the
  // RTS reply until we are CTS.
  //
  // Alternatively, the sender sends a packet blind and then waits for a reply, and the reply
  // works as CTS for the next packet.  This is a little primitive but OK for what we're
  // doing.  The reply content could be meaningful, eg, the length of the received data and a
  // checksum.  Packets need to be of a limited length.  There needs to be a timeout for the
  // reply.  It is possible to send a zero-length packet to probe for a CTS.  Just sending
  // blind always risks overwriting existing data though, which means there also needs to be
  // higher-level flow control / resend, ie, there's a higher-level protocol here too.
  // That protocol probably requires additional data to be returned.  So then the reply packet
  // needs to have space for a little payload, which also needs checksumming.  Better would
  // be to have some kind of handshake that guarantees delivery of a package in the right
  // sequence.  Ie sender sends a packet to set up a communication and receives an ack that
  // ensures that we're synchronized.  Then packets are sent in sequence and are ACKed or NAKed.
  // NAKed packets are resent until they are successful.  When the last packet has been ACKed
  // we are done.
  //
  // Setup: A B C D n n,        reply X Y Z W c c where n n is number of packets
  // Data:  E F G H n n m d ...   reply J K L M cc  where n is sequence number and m is length of packet
  //
  // We get a callback when a packet has been received and get to read the data out of the
  // packet, but we have to do so quickly, before more data arrives.  This is primitive.
  // To do better there needs to be a handshake that the data that have been sent have
  // arrived as they should and have been processed.
  //
  // Probably we have to implement a request handling.  The receiver reads a byte from the
  // device and tries to figure out if it's CTS.  If it is, it writes the data.  It then
  // reads another byte.  Since the reading implies a write it means the other data must have
  // arrived and that the correct value can be returned for the CTS.  The CTS is set once
  // all the data have been consumed.
  //
  // Really we want for the callbacks to directly pump data, we don't want for the polling
  // loop necessarily to poll?
  //
  // Transfer size is a thing?
  //
  // These stubs have to be in place, later we probably want to make better use of them!
  // The TwoWire protocol is a bit bizarre.  We get callbacks about n bytes being available.
  // We read n bytes.  If there is no more input then we can flush() the buffer to reset
  // the pointers to the beginning; if not, new input continues to arrive where we stopped
  // reading.  Eventually the buffer fills up.
  Wire1.onReceive([](int nbytes) {
  });
  Wire1.onRequest([]() {});

  // Set up the slave
  if (!Wire1.begin(I2C_SLAVE_ADDRESS, I2C_SLAVE_SDA, I2C_SLAVE_SCL, 100000)) {
    panic("Could not init wire1");
  }
  log("I2C slave initialized\n");
  delay(1000);
}
#endif

void power_peripherals_off() {
  // Do all of this unconditionally so that we can use it as a kind of fail-safe to reset
  // the peripherals.

  // Shut down I2C and remove the driver.
  Wire.end();

  // Force the I2C signals to 0V.
  gpio_pullup_dis((gpio_num_t)I2C_MASTER_SDA);
  gpio_set_direction((gpio_num_t)I2C_MASTER_SDA, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)I2C_MASTER_SDA, 0);

  gpio_pullup_dis((gpio_num_t)I2C_MASTER_SCL);
  gpio_set_direction((gpio_num_t)I2C_MASTER_SCL, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)I2C_MASTER_SCL, 0);

  // Shut off power.
  digitalWrite(POWER_ENABLE_PIN, LOW);
  peripherals_powered_on = false;
}

void reset_pir_and_mems() {
  pir_value = 0;
  mems_value = 0;
}

void sample_pir() {
#ifdef SENSE_MOTION
  pir_value |= analogRead(PIR_SENSOR_PIN);
#endif
}

void sample_mems() {
#ifdef SENSE_NOISE
  mems_value = max(mems_value, unsigned(analogRead(MIC_PIN)));
#endif
}

void get_sensor_values(SnappySenseData* data) {
  if (!peripherals_powered_on) {
    return;
  }

  data->sequence_number = sequence_number++;

  data->time = time(nullptr);

#ifdef SENSE_TEMPERATURE
  if (have_environment) {
    data->temperature = environment.getTemperature(TEMP_C);
    data->have_temperature = data->temperature != -45.0;
  }
#endif
#ifdef SENSE_HUMIDITY
  if (have_environment) {
    data->humidity = environment.getHumidity();
    data->have_humidity = data->humidity != 0;
  }
#endif
#ifdef SENSE_UV
  if (have_environment) {
    data->uv = environment.getUltravioletIntensity();
    data->have_uv = true;
  }
#endif
#ifdef SENSE_LIGHT
  if (have_environment) {
    data->lux = environment.getLuminousIntensity();
    data->have_lux = true;
  }
#endif
#ifdef SENSE_PRESSURE
  if (have_environment) {
    data->hpa = environment.getAtmospherePressure(HPA);
    data->have_hpa = data->hpa > 0;
  }
#endif
#ifdef SENSE_ALTITUDE
  if (have_environment) {
    data->elevation = environment.getElevation();
    data->have_elevation = true;
  }
#endif

#if defined(SENSE_AIR_QUALITY_INDEX) || defined(SENSE_TVOC) || defined(SENSE_CO2)
# if !defined(SENSE_HUMIDITY) || !defined(SENSE_TEMPERATURE)
#  error "Humidity and temperature required for air quality"
# endif

  if (!air_is_primed && data->have_humidity && data->have_temperature) {
    ENS160.setTempAndHum(data->temperature, data->humidity / 100);
    air_is_primed = true;
  }
  if (air_is_primed) {
    data->air_sensor_status = ENS160.getENS160Status();
    data->have_air_sensor_status = true;
    if (data->air_sensor_status != 3) {
#ifdef SENSE_AIR_QUALITY_INDEX
      data->aqi = ENS160.getAQI();
      data->have_aqi = data->aqi >= 1 && data->aqi <= 5;
#endif
#ifdef SENSE_TVOC
      data->tvoc = ENS160.getTVOC();
      data->have_tvoc = data->tvoc > 0 && data->tvoc <= 65000;
#endif
#ifdef SENSE_CO2
      data->eco2 = ENS160.getECO2();
      data->have_eco2 = data->eco2 > 400;
#endif
    }
  } else {
    data->have_air_sensor_status = false;
#ifdef SENSE_AIR_QUALITY_INDEX
    data->have_aqi = false;
#endif
#ifdef SENSE_TVOC
    data->have_tvoc = false;
#endif
#ifdef SENSE_CO2
    data->have_eco2 = false;
#endif
  }
#endif

#ifdef SENSE_MOTION
  data->motion_detected = pir_value != 0;
  data->have_motion = true;
#endif

#ifdef SENSE_NOISE
  data->noise = mems_value;
  data->have_noise = true;
#endif
}

void show_splash() {
  if (!peripherals_powered_on) {
    return;
  }

  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    knowit_logo, LOGO_WIDTH, LOGO_HEIGHT, 1);

  display.display();
}

void render_oled_view(const uint8_t *bitmap, const char* value, const char *units) {
  if (!peripherals_powered_on) {
    return;
  }
  display.clearDisplay();

  display.drawBitmap
    (0,
    (display.height() - ICON_HEIGHT) / 2,
    bitmap, ICON_WIDTH, ICON_HEIGHT, 1);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(ICON_TEXT_START_X, ICON_TEXT_START_Y);
  display.print(value);
  display.print(units);
  display.display();
}

void render_text(const char* value) {
  if (!peripherals_powered_on) {
    return;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(value);
  display.display();
}

void enter_end_state(const char* msg, bool is_error) {
  render_text("Press reset button!");
  Serial.println("Press reset button!");
  for(;;) { }
  /*NOTREACHED*/
}

#ifdef SNAPPY_PIEZO
void setup_sound() {
  // TODO: Justify these choices, which are wrong.  Probably 1 bit of resolution is fine?
  // And the question is whether ledcSetup should be before or after ledcAttachPin.
  //ledcSetup(PWM_CHAN, 4000, 16);
  ledcAttachPin(PWM_PIN, PWM_CHAN);
}

void start_note(int frequency) {
  ledcWriteTone(PWM_CHAN, frequency);
}

void stop_note() {
  ledcWrite(PWM_CHAN, 0);
}
#endif // SNAPPY_PIEZO

unsigned long entropy() {
  // Well, at least we tried.
  return micros() ^ (analogRead(PIR_SENSOR_PIN) << 8) ^ (analogRead(MIC_PIN) << 16);
}