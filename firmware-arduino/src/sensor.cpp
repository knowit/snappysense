// Sensor model.

#include "sensor.h"
#include "config.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "snappytime.h"

// The "formatters" format the various members of SnappySenseData into a buffer.  In all
// cases, `buflim` points to the address beyond the buffer.  No error is returned
// if the buffer is too small, but the operation is guaranteed not to write beyond
// the end of the buffer.

static void format_sequenceno(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%u", data.sequence_number);
}

#ifdef TIMESTAMP
static void format_time(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim-buf, "\"%s\"", format_time(data.time).c_str());
}
#endif

#ifdef SENSE_TEMPERATURE
static void format_temp(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%f", data.temperature);
}
#endif

#ifdef SENSE_HUMIDITY
static void format_humidity(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%f", data.humidity);
}
#endif

#ifdef SENSE_UV
static void format_uv(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%f", data.uv);
}
#endif

#ifdef SENSE_LIGHT
static void format_light(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%f", data.lux);
}
#endif

#ifdef SENSE_PRESSURE
static void format_pressure(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%u", data.hpa);
}
#endif

#ifdef SENSE_ALTITUDE
void format_altitude(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%f", data.elevation);
}
#endif

void format_air_sensor_status(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", data.air_sensor_status);
}

#ifdef SENSE_AIR_QUALITY_INDEX
static void format_air_quality(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", data.aqi);
}
#endif

#ifdef SENSE_TVOC
static void format_tvoc(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", data.tvoc);
}
#endif

#ifdef SENSE_CO2
static void format_co2(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", data.eco2);
}
#endif

#ifdef SENSE_MOTION
static void format_motion(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", data.motion_detected);
}
#endif

#ifdef SENSE_NOISE
static void format_noise(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", data.noise);
}
#endif

// The "displayers" format the data so that they can be shown on the unit's
// display.  There may be some information loss.  Where there's a formatter
// that would produce the same string, we use that.
// "Displayers" display for SLIDESHOW_MODE use, if the "formatted" display has too much
// information.

#ifdef SENSE_TEMPERATURE
static void display_temp(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%3.1f", data.temperature);
}
#endif

#ifdef SENSE_HUMIDITY
static void display_humidity(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%3.1f", data.humidity);
}
#endif

#ifdef SENSE_LIGHT
static void display_light(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", (int)data.lux);
}
#endif

#ifdef SENSE_ALTITUDE
static void display_altitude(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim - buf, "%d", (int)data.elevation);
}
#endif

#ifdef SLIDESHOW_MODE
#define ICON(x) x
#else
#define ICON(x) nullptr
#endif

SnappyMetaDatum snappy_metadata[] = {
  {"sequenceno",  "Sequence number",       "",    "",        nullptr,                0,
  nullptr,              format_sequenceno},
#ifdef TIMESTAMP
  {"time",        "Local time of reading", "",    "",        nullptr,                offsetof(SnappySenseData, have_time),
   nullptr,             format_time},
#endif
#ifdef SENSE_TEMPERATURE
  {"temperature", "Temperature",           "C",   "C",       ICON(temperature_icon), offsetof(SnappySenseData, have_temperature),
  display_temp,        format_temp},
#endif
#ifdef SENSE_HUMIDITY
  {"humidity",    "Humidity",              "%",   "%",       ICON(humidity_icon),    offsetof(SnappySenseData, have_humidity),
  display_humidity,    format_humidity},
#endif
#ifdef SENSE_UV
  {"uv",          "Ultraviolet intensity", "",    "mW/cm^2", ICON(uv_icon),          offsetof(SnappySenseData, have_uv),
  format_uv,           format_uv},
#endif
#ifdef SENSE_LIGHT
  {"light",       "Luminous intensity",    "lx",  "lx",      ICON(lux_icon),         offsetof(SnappySenseData, have_lux),
  display_light,       format_light},
#endif
#ifdef SENSE_PRESSURE
  {"pressure",    "Atmospheric pressure",  "hpa", "hpa",     ICON(hpa_icon),         offsetof(SnappySenseData, have_hpa),
  format_pressure,     format_pressure},
#endif
#ifdef SENSE_ALTITUDE
  {"altitude",    "Altitude",              "m",   "m",       ICON(elevation_icon),   offsetof(SnappySenseData, have_altitude),
  display_altitude,    format_altitude},
#endif
  {"airsensor",   "Air sensor status",     "",    "",        nullptr,                offsetof(SnappySenseData, have_air_sensor_status),
  nullptr,             format_air_sensor_status},
#ifdef SENSE_AIR_QUALITY_INDEX
  {"airquality",  "Air quality index",     "",    "",        ICON(aqi_icon),         offsetof(SnappySenseData, have_aqi),
  format_air_quality,  format_air_quality},
#endif
#ifdef SENSE_TVOC
  {"tvoc",        "Concentration of total volatile organic compounds",
                                           "ppb", "ppb",     ICON(aqi_icon),         offsetof(SnappySenseData, have_tvoc),
   format_tvoc,         format_tvoc},
#endif
#ifdef SENSE_CO2
  {"co2",         "Carbon dioxide equivalent concentration",
                                           "ppm", "ppm",     ICON(co2_icon),         offsetof(SnappySenseData, have_eco2),
   format_co2,          format_co2},
#endif
#ifdef SENSE_MOTION
  { "motion",     "Motion detected",       "",    "",        ICON(motion_icon),      offsetof(SnappySenseData, have_motion),
  format_motion,       format_motion},
#endif
#ifdef SENSE_NOISE
  {"noise",       "Noise value",           "",    "",        ICON(noise_icon),       offsetof(SnappySenseData, have_noise),
  format_noise,        format_noise},
#endif
  {nullptr,       nullptr,                 "",    "",        nullptr,                0,
   nullptr,             nullptr}
};

// The JSON data format is defined by aws-iot-backend/mqtt-protocol.md
String format_readings_as_json(const SnappySenseData& data) {
  // TODO: Issue 17: for production code we have to handle OOM all the way down, see
  // comments below.  String::operator+= does not deal with that.
  String buf;
  buf += '{';
  buf += "\"location\":\"";
  buf += location_name();
  buf += '"';
  for ( SnappyMetaDatum* r = snappy_metadata; r->json_key != nullptr; r++ ) {
    buf += ',';
    buf += '"';
    buf += r->json_key;
    buf += '"';
    buf += ':';
    // Skip data that are not valid
    if (r->flag_offset > 0 &&
        !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&data) + r->flag_offset)) {
      continue;
    }
    char tmp[256];
    r->format(data, tmp, tmp+sizeof(tmp));
    buf += tmp;
  }
  buf += '}';
  return buf;
}

void ReadSensorsTask::execute(SnappySenseData* data) {
  get_sensor_values(data);
}

void EnableDeviceTask::execute(SnappySenseData*) {
  set_device_enabled(flag);
}

void RunActuatorTask::execute(SnappySenseData*) {
  // TODO: Issue 21: Display something, if the display is going
  // TODO: Issue 22: Manipulate an actuator, if we have one (we don't, really)
  if (actuator.equals("temperature")) {
    if (reading >= ideal+2) {
      log("It's HOT in here!\n");
    } else if (reading <= ideal-2) {
      log("I'm starting to feel COLD!\n");
    }
  } else if (actuator.equals("airquality")) {
    if (reading > ideal) {
      log("The air is pretty BAD in here!\n");
    }
  }
  // and so on
}

void PowerOnTask::execute(SnappySenseData*) {
  power_peripherals_on();
}

void PowerOffTask::execute(SnappySenseData*) {
  power_peripherals_off();
}
