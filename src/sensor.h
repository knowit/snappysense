// Sensor model.

#ifndef sensor_h_included
#define sensor_h_included

#include "main.h"

// This is the model of the sensor unit.

typedef struct SnappySenseData {
  // Altitude of device, meters above (below) sea level
  float elevation;

  // Relative humidity, percent
  float humidity;

  // Degrees Celsius
  float temperature;

  // Air pressure, in 10^2 Pascal
  uint16_t hpa;

  // Illumninance, in lux
  float lux;

  // Ultraviolet radiation, in mW / cm^2
  float uv;

  // True or false.  TODO: So why not boolean
  uint8_t air_sensor_status;

  // AQI: 1-Excellent, 2-Good, 3-Moderate, 4-Poor, 5-Unhealthy
  uint8_t aqi;

  // Total volatile organic compounds, 0–65000, ppb
  uint16_t tvoc;

  // CO2, 400–65000, ppm
  // Five levels: Excellent(400 - 600), Good(600 - 800), Moderate(800 - 1000), 
  //              Poor(1000 - 1500), Unhealthy(> 1500)
  uint16_t eco2;

  // TODO: Unit?
  uint16_t noise;

  // Passive motion sensor
  // TODO: Unit?
  bool pir;
} SnappySenseData;

// Sensor metadata.  There is one row in the metadata table for each field in the model
// (though not necessarily in the same order).  The metadata can be used to format
// and describe the fields in various ways.

typedef struct {
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

  // `display` is for the unit's display when running standalone, it loses some information.
  // This may be null, if it is it's because we don't want to display this.
  void (*display)(const SnappySenseData& data, char* buf, char* buflim);

  // `format` is for the view command, JSON data extraction, and so on - all information
  // is preserved.
  void (*format)(const SnappySenseData& data, char* buf, char* buflim);
} SnappyMetaDatum;

// The metadata table is terminated by a row where json_key == nullptr.
extern SnappyMetaDatum snappy_metadata[];

String format_readings_as_json(const SnappySenseData& data, unsigned sequence_number);

#endif // !sensor_h_included
