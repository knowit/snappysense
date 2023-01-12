// Sensor model.

#include "sensor.h"
#include "config.h"
#include "device.h"
#include "icons.h"
#include "log.h"
#include "snappytime.h"

SnappySenseData snappy;

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

static void format_motion(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.motion_detected);
}

#ifdef READ_NOISE
static void format_noise(const SnappySenseData& data, char* buf, char* buflim) { 
  snprintf(buf, buflim - buf, "%d", data.noise);
}
#endif

// The "displayers" format the data so that they can be shown on the unit's
// display.  There may be some information loss.  Where there's a formatter
// that would produce the same string, we use that.
// "Displayers" display for DEMO_MODE use, if the "formatted" display has too much
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

#ifdef DEMO_MODE
#define ICON(x) x
#else
#define ICON(x) nullptr
#endif

SnappyMetaDatum snappy_metadata[] = {
  {"sequenceno",  "Sequence number",       "",    "",        nullptr,                nullptr,             format_sequenceno},
#ifdef TIMESTAMP
  {"time",        "Local time of reading", "",    "",        nullptr,                nullptr,             format_time},
#endif
  {"temperature", "Temperature",           "C",   "C",       ICON(temperature_icon), display_temp,        format_temp},
  {"humidity",    "Humidity",              "%",   "%",       ICON(humidity_icon),    display_humidity,    format_humidity},
  {"uv",          "Ultraviolet intensity", "",    "mW/cm^2", ICON(uv_icon),          format_uv,           format_uv},
  {"light",       "Luminous intensity",    "lx",  "lx",      ICON(lux_icon),         display_light,       format_light},
  {"pressure",    "Atmospheric pressure",  "hpa", "hpa",     ICON(hpa_icon),         format_pressure,     format_pressure},
  {"altitude",    "Altitude",              "m",   "m",       ICON(elevation_icon),   display_altitude,    format_altitude},
  {"airsensor",   "Air sensor status",     "",    "",        nullptr,                nullptr,             format_air_sensor_status},
  {"airquality",  "Air quality index",     "",    "",        ICON(aqi_icon),         format_air_quality,  format_air_quality},
  {"tvoc",        "Concentration of total volatile organic compounds",
                                           "ppb", "ppb",     ICON(aqi_icon),         format_tvoc,         format_tvoc},
  {"co2",         "Carbon dioxide equivalent concentration",
                                           "ppm", "ppm",     ICON(co2_icon),         format_co2,          format_co2},
  { "motion",     "Motion detected",       "",    "",        ICON(motion_icon),      format_motion,       format_motion},
#ifdef READ_NOISE
  {"noise",       "Noise value",           "",    "",        ICON(noise_icon),       format_noise,        format_noise},
#endif
  {nullptr,       nullptr,                 "",    "",        nullptr,                nullptr,             nullptr}
};

String format_readings_as_json(const SnappySenseData& data) {
  // TODO, maybe use ArduinoJSON here, we're pulling that in for mqtt anyway.
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
  // TODO: Display something, if the display is going
  // TODO: Manipulate an actuator, if we have one (we don't, really)
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
