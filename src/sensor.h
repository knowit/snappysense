// Sensor model.

#ifndef sensor_h_included
#define sensor_h_included

#include "main.h"
#include "microtask.h"

// This is the model of the sensor unit.

struct SnappySenseData {
  // The sequence number is useful for calibration, bug fixing, etc.  It is
  // set from a global variable when a reading is obtained.  It will wrap
  // around silently.
  unsigned sequence_number;

#ifdef TIMESTAMP
  // Current local time when the reading was taken
  struct tm time;
#endif

#ifdef SENSE_ALTITUDE
  // Altitude of device, meters above (below) sea level
  float elevation;
#endif

#ifdef SENSE_HUMIDITY
  // Relative humidity, percent
  float humidity;
#endif

#ifdef SENSE_TEMPERATURE
  // Degrees Celsius
  float temperature;
#endif

#ifdef SENSE_PRESSURE
  // Air pressure, in 10^2 Pascal
  uint16_t hpa;
#endif

#ifdef SENSE_LIGHT
  // Illumninance, in lux
  float lux;
#endif

#ifdef SENSE_UV
  // Ultraviolet radiation, in mW / cm^2
  float uv;
#endif

  // Status code from the device: 0=normal, 1=warmup, 2=startup, 3=invalid
  uint8_t air_sensor_status;

#ifdef SENSE_AIR_QUALITY_INDEX
  // AQI: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy
  uint8_t aqi;
#endif

#ifdef SENSE_TVOC
  // Total volatile organic compounds, 0–65000, ppb
  uint16_t tvoc;
#endif

#ifdef SENSE_CO2
  // CO2, 400–65000, ppm
  // Five levels: Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000),
  //              Poor(1000 - 1500), Unhealthy(> 1500)
  uint16_t eco2;
#endif

#ifdef SENSE_NOISE
  // The value looks like it's the voltage reading times 1000: the lower limit is
  // about 1500, which is what the data sheet says corresponds to the quiescent
  // state at 1.5V.  As the environment becomes noisier, the numbers become
  // higher, reaching into the 2200 range.  Some readings were seen around 900
  // and 4100, but not clear what these were.
  uint16_t noise;
#endif

#ifdef SENSE_MOTION
  // Passive motion sensor.  Unit: no movement / movement.
  bool motion_detected;
#endif
};

// Sensor metadata.  There is one row in the metadata table for each field in the model
// (though not necessarily in the same order).  The metadata can be used to format
// and describe the fields in various ways.

struct SnappyMetaDatum {
  // The json key is unique, and must be simple alphanumeric, no punctuation.
  const char* json_key;

  // Anything at all, this is for human consumption
  const char* explanatory_text;

  // A name for the measurement unit that is suitably short for the built-in display.
  // This is a hack, but sometimes the proper unit names are too long.
  const char* display_unit;

  // The measurement unit, eg "C" for degrees celsius
  const char* unit_text;

  // The icon may be null, otherwise it's a pointer to a bitmap
  const unsigned char* icon;

  // `display` is for the unit's display when running SLIDESHOW_MODE, it loses some information.
  // This may be null, if it is it's because we don't want to display this.
  void (*display)(const SnappySenseData& data, char* buf, char* buflim);

  // `format` is for the view command, JSON data extraction, and so on - all information
  // is preserved.
  void (*format)(const SnappySenseData& data, char* buf, char* buflim);
};

// The metadata table is terminated by a row where json_key == nullptr.
extern SnappyMetaDatum snappy_metadata[];

String format_readings_as_json(const SnappySenseData& data);

// This task is used both for periodic reading and one-shot reading.
class ReadSensorsTask final : public MicroTask {
public:
  const char* name() override {
    return "Read sensors";
  }
  virtual bool only_when_device_enabled() {
    return true;
  }
  void execute(SnappySenseData* data) override;
};

// A one-shot task that enables or disables the device.  When the device is disabled,
// tasks that respond `true` to only_when_device_enabled() are skipped.
class EnableDeviceTask final : public MicroTask {
  bool flag;
public:
  EnableDeviceTask(bool flag) : flag(flag) {}
  const char* name() override {
    return "Enable/disable device";
  }
  void execute(SnappySenseData*) override;
};

// A one-shot task that interacts with an actuator to change an environmental factor.
class RunActuatorTask final : public MicroTask {
  String actuator;
  double reading;
  double ideal;
public:
  RunActuatorTask(String&& actuator, double reading, double ideal)
    : actuator(std::move(actuator)), reading(reading), ideal(ideal)
  {}
  const char* name() override {
    return "Run actuator";
  }
  virtual bool only_when_device_enabled() {
    return true;
  }
  void execute(SnappySenseData*) override;
};

class PowerOnTask final : public MicroTask {
public:
  const char* name() override {
    return "Power on";
  }
  void execute(SnappySenseData*) override;
};

class PowerOffTask final : public MicroTask {
public:
  const char* name() override {
    return "Power off";
  }
  void execute(SnappySenseData*) override;
};
#endif // !sensor_h_included
