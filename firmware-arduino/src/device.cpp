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
#include "snappytime.h"
#include "Wire.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobot_ENS160.h>
#include <DFRobot_EnvironmentalSensor.h>
#include <esp32-hal-ledc.h>

// SnappySense 1.x.y device definition

// Pin definitions
#define POWER_ENABLE_PIN A0
#define BUTTON_PIN A1  // WAKE on 1.0.0, BTN1 on 1.1.0
#if defined(HARDWARE_1_0_0)
# define PIR_SENSOR_PIN A4
# define MIC_PIN A5
#elif defined(HARDWARE_1_1_0)
# define PIR_SENSOR_PIN A2
# define MIC_PIN A3
#else
# error "Fix your hardware definitions"
#endif
#define I2C_SDA SDA
#define I2C_SCL SCL
#define PWM_PIN T8  // Tentative: ADC1 CH5, pin IO33 aka pin T8
#define PWM_CHAN 5

// I2C addresses
#define I2C_OLED_ADDRESS 0x3C
#define I2C_AIR_ADDRESS  0x53
#define I2C_DHT_ADDRESS  0x22

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
static bool have_environment = false;
static bool have_air = false;
static bool air_is_primed = false;

static void button_handler() {
  put_main_event_from_isr(digitalRead(BUTTON_PIN) ? EvCode::BUTTON_DOWN : EvCode::BUTTON_UP);
}

// This must NOT depend on the configuration because the configuration may not
// have been read at this point, see main.cpp.

void device_setup(bool* do_interactive_configuration) {
  // Always connect serial on startup
  Serial.begin(115200);
#ifdef LOGGING
  // Connect the serial port whether anyone's listening or not.  This is in favor
  // of being able to plug in a serial cable to check on what's going on, after startup.
  set_log_stream(&Serial);
#endif

  // Set up io pins
  pinMode(POWER_ENABLE_PIN, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  // Bring up all peripherals
  power_peripherals_on();

  log("Device initialized\n");

#ifdef WEB_CONFIGURATION
  // To enter configuration mode, press and hold the WAKE/BTN1 button and then press and release
  // the reset button.  For more information, see CONFIG.md at the root of the repo.
  if (digitalRead(BUTTON_PIN)) {
    delay(1000);
    if (digitalRead(BUTTON_PIN)) {
      *do_interactive_configuration = true;
    }
  }
#endif

  attachInterrupt(BUTTON_PIN, button_handler, CHANGE);
}

void power_peripherals_on() {
  if (!peripherals_powered_on) {
    // turn on peripheral power, must be on for i2c to work!
    digitalWrite(POWER_ENABLE_PIN, HIGH);
    // wait until peripherals are stable.  100ms is not enough (issue #14), and 1000ms does not
    // really seem to be a hardship, so why not.
    delay(1000);

    // init i2c
    // Note the (int) cast, some versions of the ESP32 libs need this, ref
    // https://github.com/espressif/arduino-esp32/issues/6616#issuecomment-1184167285
    Wire.begin((int) I2C_SDA, I2C_SCL);

    have_environment = environment.begin() == 0;
    have_air = ENS160.begin() == 0;

    // init oled display
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    display.begin(SSD1306_SWITCHCAPVCC, I2C_OLED_ADDRESS);
    peripherals_powered_on = true;
  }
}

void power_peripherals_off() {
  // Do this unconditionally so that we can use it as a kind of fail-safe to reset
  // the peripherals.
  digitalWrite(POWER_ENABLE_PIN, LOW);
  peripherals_powered_on = false;
}

int probe_i2c_devices(Stream* stream) {
  byte error, address;
  uint8_t num = 0;

  if (!peripherals_powered_on) {
    stream->println("Peripherals are presently powered off");
    return num;
  }

  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of the Wire.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      ++num;
      stream->print("I2C device found at address 0x");
      if (address<16)
        stream->print("0");
      stream->println(address,HEX);
    }
  }
  return num;
}

// The sequence_number is not very useful any more, but it's a fallback for when we don't have a time
// stamp, and may help us out when the time changes under our feet.
static unsigned sequence_number;

void get_sensor_values(SnappySenseData* data) {
  if (!peripherals_powered_on) {
    return;
  }

  memset(data, 0, sizeof(SnappySenseData));
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
  }
#endif

#ifdef SENSE_MOTION
  data->motion_detected = (analogRead(PIR_SENSOR_PIN) != 0 ? true : false);
  data->have_motion = true;
#endif

#ifdef SENSE_NOISE
  data->noise = analogRead(MIC_PIN);
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