// Interface to actual hardware device

#include "device.h"
#include "icons.h"
#include "log.h"
#include "sensor.h"
#include "Wire.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DFRobot_ENS160.h>
#include <DFRobot_EnvironmentalSensor.h>

// SnappySense 1.0.0 device definition

// Pin definitions
#define POWER_ENABLE_PIN A0
#define WAKEUP_PIN A1
#define PIR_SENSOR_PIN A4
#define MIC_PIN A5
#define I2C_SDA SDA
#define I2C_SCL SCL

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

static Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
static DFRobot_ENS160_I2C ENS160(&Wire, I2C_AIR_ADDRESS);
static DFRobot_EnvironmentalSensor environment(I2C_DHT_ADDRESS, /*pWire = */&Wire);

void device_setup() {
  Serial.begin(115200);
#ifdef LOGGING
  // This could be something else, and it could be configurable.
  // FIXME: If the serial port is not connected, this should do nothing.
  // FIXME: Logging could also be to a buffer and the log could be requested interactively.
  set_log_stream(&Serial);
#endif

  // set up io pins
  pinMode(POWER_ENABLE_PIN, OUTPUT);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(MIC_PIN, INPUT);

  // turn on peripheral power, must be on for i2c to work!
  digitalWrite(POWER_ENABLE_PIN, HIGH);
  delay(100);

  // init i2c
  // Note the (int) cast, some versions of the ESP32 libs need this, ref
  // https://github.com/espressif/arduino-esp32/issues/6616#issuecomment-1184167285
  Wire.begin((int) I2C_SDA, I2C_SCL);

  // init oled display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, I2C_OLED_ADDRESS);

  log("Device initialized\n");
}

void power_on() {
  digitalWrite(POWER_ENABLE_PIN, HIGH);
}

void power_off() {
  digitalWrite(POWER_ENABLE_PIN, LOW);
}

int probe_i2c_devices(Stream* stream) {
  byte error, address;
  uint8_t num = 0;
 
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
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

void get_sensor_values(SnappySenseData* data) {
  environment.begin();
  data->temperature = environment.getTemperature(TEMP_C);
  data->humidity = environment.getHumidity();
  data->uv = environment.getUltravioletIntensity();
  data->lux = environment.getLuminousIntensity();
  data->hpa = environment.getAtmospherePressure(HPA);
  data->elevation = environment.getElevation();

  ENS160.begin();
  ENS160.setPWRMode(ENS160_STANDARD_MODE);
  ENS160.setTempAndHum(/*temperature=*/data->temperature, /*humidity=*/data->humidity);
  data->air_sensor_status = ENS160.getENS160Status();
  data->aqi = ENS160.getAQI();
  data->tvoc = ENS160.getTVOC();
  data->eco2 = ENS160.getECO2();

  // There's a conflict between the WiFi and the ADC.
  data->pir = (analogRead(PIR_SENSOR_PIN) != 0 ? true : false);
#if defined(READ_NOISE)
  data->noise = analogRead(MIC_PIN);
#endif
}

void show_splash() {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    knowit_logo, LOGO_WIDTH, LOGO_HEIGHT, 1);

  display.display();
}

#ifdef STANDALONE
void render_oled_view(const uint8_t *bitmap, const char* value, const char *units) {
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
#endif
