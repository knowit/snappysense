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

static void format_timestamp(const SnappySenseData& data, char* buf, char* buflim) {
  snprintf(buf, buflim-buf, "%s", format_timestamp(data.time).c_str());
}

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
// "Displayers" display for slideshow use, if the "formatted" display has too much
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

SnappyMetaDatum snappy_metadata[] = {
  {.json_key         = "sequenceno",
   .explanatory_text = "Sequence number",
   .display_unit     = "",
   .unit_text        = "",
   .icon             = nullptr,
   .flag_offset      = 0,
   .display         = nullptr,
   .format           = format_sequenceno },
  {.json_key         = "sent",
   .explanatory_text = "Local time of observation",
   .display_unit     = "",
   .unit_text        = "",
   .icon             = nullptr,
   .flag_offset      = 0,
   .display         = nullptr,
   .format           = format_timestamp},
#ifdef SENSE_TEMPERATURE
  {.json_key         = "temperature",
   .explanatory_text = "Temperature",
   .display_unit     = "C",
   .unit_text        = "C",
   .icon             = temperature_icon,
   .flag_offset      = offsetof(SnappySenseData, have_temperature),
   .display         = display_temp,
   .format           = format_temp},
#endif
#ifdef SENSE_HUMIDITY
  {.json_key         = "humidity",
   .explanatory_text = "Humidity",
   .display_unit     = "%",
   .unit_text        = "%",
   .icon             = humidity_icon,
   .flag_offset      = offsetof(SnappySenseData, have_humidity),
   .display          = display_humidity,
   .format           = format_humidity},
#endif
#ifdef SENSE_UV
  {.json_key         = "uv",
   .explanatory_text = "Ultraviolet intensity",
   .display_unit     = "",
   .unit_text        = "mW/cm^2",
   .icon             = uv_icon,
   .flag_offset      = offsetof(SnappySenseData, have_uv),
   .display          = format_uv,
   .format           = format_uv},
#endif
#ifdef SENSE_LIGHT
  {.json_key         = "light",
   .explanatory_text = "Luminous intensity",
   .display_unit     = "lx",
   .unit_text        = "lx",
   .icon             = lux_icon,
   .flag_offset      = offsetof(SnappySenseData, have_lux),
   .display          = display_light,
   .format           = format_light},
#endif
#ifdef SENSE_PRESSURE
  {.json_key         = "pressure",
   .explanatory_text = "Atmospheric pressure",
   .display_unit     = "hpa",
   .unit_text        = "hpa",
   .icon             = hpa_icon,
   .flag_offset      = offsetof(SnappySenseData, have_hpa),
   .display          = format_pressure,
   .format           = format_pressure},
#endif
#ifdef SENSE_ALTITUDE
  {.json_key         = "altitude",
   .explanatory_text = "Altitude",
   .display_unit     = "m",
   .unit_text        = "m",
   .icon             = elevation_icon,
   .flag_offset      = offsetof(SnappySenseData, have_altitude),
   .display          = display_altitude,
   .format           = format_altitude},
#endif
  {.json_key         = "airsensor",
   .explanatory_text = "Air sensor status",
   .display_unit     = "",
   .unit_text        = "",
   .icon             = nullptr,
   .flag_offset      = offsetof(SnappySenseData, have_air_sensor_status),
   .display          = nullptr,
   .format           = format_air_sensor_status},
#ifdef SENSE_AIR_QUALITY_INDEX
  {.json_key         = "airquality",
   .explanatory_text = "Air quality index",
   .display_unit     = "",
   .unit_text        = "",
   .icon             = aqi_icon,
   .flag_offset      = offsetof(SnappySenseData, have_aqi),
   .display          = format_air_quality,
   .format           = format_air_quality},
#endif
#ifdef SENSE_TVOC
  {.json_key         = "tvoc",
   .explanatory_text = "Concentration of total volatile organic compounds",
   .display_unit     = "ppb",
   .unit_text        = "ppb",
   .icon             = aqi_icon,
   .flag_offset      = offsetof(SnappySenseData, have_tvoc),
   .display          = format_tvoc,
   .format           = format_tvoc},
#endif
#ifdef SENSE_CO2
  {.json_key         = "co2",
   .explanatory_text = "Carbon dioxide equivalent concentration",
   .display_unit     = "ppm",
   .unit_text        = "ppm",
   .icon             = co2_icon,
   .flag_offset      = offsetof(SnappySenseData, have_eco2),
   .display          = format_co2,
   .format           = format_co2},
#endif
#ifdef SENSE_MOTION
  {.json_key         = "motion",
   .explanatory_text = "Motion detected",
   .display_unit     = "",
   .unit_text        = "",
   .icon             = motion_icon,
   .flag_offset      = offsetof(SnappySenseData, have_motion),
   .display          = format_motion,
   .format           = format_motion},
#endif
#ifdef SENSE_NOISE
  {.json_key         = "noise",
   .explanatory_text = "Noise value",
   .display_unit     = "",
   .unit_text        = "",
   .icon             = noise_icon,
   .flag_offset      = offsetof(SnappySenseData, have_noise),
   .display          = format_noise,
   .format           = format_noise},
#endif
  {.json_key         = nullptr,
   .explanatory_text = nullptr,
   .display_unit     = "",
   .unit_text        = "",
   .icon             = nullptr,
   .flag_offset      = 0,
   .display          = nullptr,
   .format           = nullptr}
};

// The JSON data format is defined by aws-iot-backend/MQTT-PROTOCOL.md
String format_readings_as_json(const SnappySenseData& data) {
  // TODO: Issue 17: for production code we have to handle OOM all the way down, see
  // comments below.  String::operator+= does not deal with that.
  String buf;
  buf += '{';
  buf += "\"location\":\"";
  buf += location_name();
  buf += '"';
  for ( SnappyMetaDatum* r = snappy_metadata; r->json_key != nullptr; r++ ) {
    // Skip data that are not valid
    if (r->flag_offset > 0 &&
        !*reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&data) + r->flag_offset)) {
      continue;
    }
    buf += ',';
    buf += '"';
    // This is a hack.  Factor names are prefixed by F# to avoid name clashes, while fields like
    // sent and sequenceno should not be prefixed.  The flag_offset doubles of an indicator for
    // whether the prefix is needed.
    if (r->flag_offset > 0) {
      buf += "F#";
    }
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

static TimerHandle_t monitoring_timer;
static bool monitoring_is_on = false;

void monitoring_timer_tick(TimerHandle_t t) {
  if (monitoring_is_on) {
    put_main_event(EvCode::MONITOR_TICK);
  }
}

void start_monitoring() {
  if (monitoring_timer == nullptr) {
    monitoring_timer = xTimerCreate("monitor", 1, pdFALSE, nullptr, monitoring_timer_tick);
  }
  // Wait 15s to let sensors warm up.  The monitoring window should be longer than this!
  // FIXME: 15s not 5
  xTimerChangePeriod(monitoring_timer, pdMS_TO_TICKS(5000), portMAX_DELAY);
  monitoring_is_on = true;
}

static void monitoring_report() {
  SnappySenseData* data = new SnappySenseData();
  get_sensor_values(data);
  put_main_event(EvCode::MONITOR_DATA, data);
}

void monitoring_tick() {
  if (monitoring_is_on) {
    monitoring_is_on = false;
    monitoring_report();
    // Don't restart the timer.  In the future we'll have more complicated logic for
    // handling the MEMS and the motion sensor.
  }
}
void stop_monitoring() {
  if (monitoring_is_on) {
    // Somebody asked to stop before the timer expired
    monitoring_is_on = false;
    xTimerStop(monitoring_timer, portMAX_DELAY);
    monitoring_report();
  }
}

#ifdef MQTT_COMMAND_MESSAGES
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
#endif
