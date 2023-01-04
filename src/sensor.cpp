// Sensor model.

#include "sensor.h"
#include "config.h"
#include "icons.h"
#include "snappytime.h"

SnappySenseData snappy;

// The "formatters" format the various members of SnappySenseData into a buffer.  In all
// cases, `buflim` points to the address beyond the buffer.  No error is returned
// if the buffer is too small, but the operation is guaranteed not to write beyond
// the end of the buffer.

static void format_temp(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%f", data.temperature);
}

static void format_humidity(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%f", data.humidity);
}

static void format_uv(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%f", data.uv);
}

static void format_light(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%f", data.lux);
}

static void format_pressure(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%u", data.hpa);
}

void format_altitude(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%f", data.elevation);
}

void format_air_sensor_status(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.air_sensor_status);
}

static void format_air_quality(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.aqi);
}

static void format_tvoc(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.tvoc);
}

static void format_co2(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.eco2);
}

static void format_pir(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.pir);
}

static void format_noise(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.noise);
}

// The "displayers" format the data so that they can be shown on the unit's
// display.  There may be some information loss.  Where there's a formatter
// that would produce the same string, we use that.
// "Displayers" display for standalone use, if the "formatted" display has too much
// information.

static void display_temp(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%3.1f", data.temperature);
}

static void display_humidity(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%3.1f", data.humidity);
}

static void display_light(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", (int)data.lux);
}

static void display_altitude(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", (int)data.elevation);
}

#ifdef STANDALONE
#define ICON(x) x
#else
#define ICON(x) nullptr
#endif

SnappyMetaDatum snappy_metadata[] = {
  {"temperature", "Temperature",           "C",   "C",      ICON(temperature_icon), display_temp,        format_temp},
  {"humidity",    "Humidity",              "%",   "%",      ICON(humidity_icon),    display_humidity,    format_humidity},
  {"uv",          "Ultraviolet intensity", "",    "mw/cm2", ICON(uv_icon),          format_uv,           format_uv},
  {"light",       "Luminous intensity",    "lx",  "lx",     ICON(lux_icon),         display_light,       format_light},
  {"pressure",    "Atmospheric pressure",  "hpa", "hpa",    ICON(hpa_icon),         format_pressure,     format_pressure},
  {"altitude",    "Altitude",              "m",   "m",      ICON(elevation_icon),   display_altitude,    format_altitude},
  {"airsensor",   "Air sensor status",     "",    "",       nullptr,                nullptr,             format_air_sensor_status},
  {"airquality",  "Air quality index",     "",    "",       ICON(aqi_icon),         format_air_quality,  format_air_quality},
  {"tvoc",        "Concentration of total volatile organic compounds",
                                           "ppb", "ppb",    ICON(aqi_icon),         format_tvoc,         format_tvoc},
  {"co2",         "Carbon dioxide equivalent concentration",
                                           "ppm", "ppm",    ICON(co2_icon),         format_co2,          format_co2},
  { "pir",        "PIR value",             "",    "",       ICON(pir_icon),         format_pir,          format_pir},
  {"noise",       "Noise value",           "",    "",       ICON(noise_icon),       format_noise,        format_noise},
  {nullptr,       nullptr,                 "",    "",       nullptr,                nullptr,             nullptr}
};

String format_readings_as_json(const SnappySenseData& data, unsigned sequence_number) {
  // TODO, maybe use ArduinoJSON here.
  // TODO, for production code we have to handle OOM all the way down, see comments below.
  // String::operator+= does not deal with that.
  String buf;
  buf += '{';
  buf += "\"location\":\"";
  buf += location_name();
  buf += "\",\"sequenceno\":";
  buf += sequence_number;
#ifdef TIMESTAMP
  // For the time stamp the information we're interested in is the local time,
  // because we want to correlate time of day and week day with readings.  And we also
  // want statistics over time, in a totally ordered list.
  // It's easier (if less compact) to extract those fields individually than
  // to deal with an encoded value.
  struct tm lt = snappy_local_time();
  buf += ",\"year\":";
  buf += lt.tm_year + 1900;  // year number
  buf += ",\"month\":";
  buf += lt.tm_mon + 1;      // month, 1-12
  buf += ",\"day\":";
  buf += lt.tm_mday;         // day of the month, 1-31
  buf += ",\"weekday\":";
  buf += lt.tm_wday + 1;     // day of the week, 1-7, 1 is sunday
  buf += ",\"hour\":";
  buf += lt.tm_hour;         // hour, 0-23
  buf += ",\"minute\":";
  buf += lt.tm_min;          // minute, 0-59
#endif
  for ( SnappyMetaDatum* r = snappy_metadata; r->json_key != nullptr; r++ ) {
    buf += ',';
    buf += '"';
    buf += r->json_key;
    buf += '"';
    buf += ':';
    char tmp[256];
    r->format(data, tmp, tmp+sizeof(tmp));
    buf += tmp;
  }
  buf += '}';  
  return buf;
}