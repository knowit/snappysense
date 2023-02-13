/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "sensor.h"

#include "device.h"
#include "sound_player.h"
#include "sound_sampler.h"

/* Sensor variables */
bool have_calibrated_sen0514 = false;

sensor_state_t sensor;

/* Trigger sensor readings */
static TimerHandle_t sensor_clock = NULL;
#define MONITORING_INTERVAL_S 15

/* Trigger the closing of the monitoring window */
static TimerHandle_t monitoring_clock = NULL;
#define MONITORING_WINDOW_S 10

static void sensor_clock_callback(TimerHandle_t t) {
  snappy_event_t ev = EV_SENSOR_CLOCK;
  xQueueSend(snappy_event_queue, &ev, portMAX_DELAY);
}

static void monitoring_clock_callback(TimerHandle_t t) {
  snappy_event_t ev = EV_MONITORING_CLOCK;
  xQueueSend(snappy_event_queue, &ev, portMAX_DELAY);
}

void sensor_begin() {
  /* Create a clock tick used to drive device readings */
  /* TODO: Should we have this instead separate windows by this interval and not
     just blindly start new monitoring this often? */
  sensor_clock = xTimerCreate("sensor", pdMS_TO_TICKS(MONITORING_INTERVAL_S*1000),
                              /* restart= */ pdTRUE,
                              NULL, sensor_clock_callback);
  xTimerStart(sensor_clock, portMAX_DELAY);

  /* This one-shot timer is started when the monitoring window opens and signals the
     close of the window. */
  monitoring_clock = xTimerCreate("monitor", pdMS_TO_TICKS(MONITORING_WINDOW_S*1000),
                                  /* restart= */ pdFALSE,
                                  NULL, monitoring_clock_callback);
}

/* The following functions read specific sensors when feature ifdefs are enabled without checking
   for the device being enabled, since there are no alternative sensors to be read.  Thus turning
   off e.g. SNAPPY_I2C_SEN0500 without also turning off the factors that that sensor reads
   (SNAPPY_READ_TEMPERATURE and so on) will cause compilation errors here.  This feels OK to me - if
   you disable that device without disabling the factors that can only be read by the device without
   providing an alternative device you're doing it wrong. */

void open_monitoring_window() {
  LOG("Monitoring window opens");

# ifdef SNAPPY_READ_TEMPERATURE
  sensor.have_temperature =
    have_sen0500 &&
    dfrobot_sen0500_get_temperature(&sen0500, DFROBOT_SEN0500_TEMP_C, &sensor.temperature) &&
    sensor.temperature != -45.0;
  if (sensor.have_temperature) {
    LOG("Temperature = %.2f", sensor.temperature);
  }
# endif

# ifdef SNAPPY_READ_HUMIDITY
  sensor.have_humidity =
    have_sen0500 &&
    dfrobot_sen0500_get_humidity(&sen0500, &sensor.humidity) &&
    sensor.humidity != 0.0;
  if (sensor.have_humidity) {
    LOG("Humidity = %.2f", sensor.humidity);
  }
# endif

# ifdef SNAPPY_READ_PRESSURE
  sensor.have_atmospheric_pressure =
    have_sen0500 &&
    dfrobot_sen0500_get_atmospheric_pressure(&sen0500, DFROBOT_SEN0500_PRESSURE_HPA,
					     &sensor.atmospheric_pressure) &&
    sensor.atmospheric_pressure != 0;
  if (sensor.have_atmospheric_pressure) {
    LOG("Pressure = %u", sensor.atmospheric_pressure);
  }
# endif

# ifdef SNAPPY_READ_UV_INTENSITY
  sensor.have_uv_intensity =
    have_sen0500 &&
    dfrobot_sen0500_get_ultraviolet_intensity(&sen0500, &sensor.uv_intensity) &&
    sensor.uv_intensity != 0.0;
  if (sensor.have_uv_intensity) {
    LOG("UV intensity = %.2f", sensor.uv_intensity);
  }
# endif

# ifdef SNAPPY_READ_LIGHT_INTENSITY
  sensor.have_luminous_intensity =
    have_sen0500 &&
    dfrobot_sen0500_get_luminous_intensity(&sen0500, &sensor.luminous_intensity) &&
    sensor.luminous_intensity != 0.0;
  if (sensor.have_luminous_intensity) {
    LOG("Luminous intensity = %.2f", sensor.luminous_intensity);
  }
# endif

#if defined(SNAPPY_READ_CO2) || defined(SNAPPY_READ_VOLATILE_ORGANICS) || defined(SNAPPY_READ_AIR_QUALITY_INDEX)
  dfrobot_sen0514_status_t stat;

  /* TODO: It's far from clear *when* this calibration should occur - whether it's every time we
     want to read the device or just the first time, or when temp and humidity change "a lot".  */
  if (have_sen0514 &&
      sensor.have_temperature &&
      sensor.have_humidity &&
      !have_calibrated_sen0514 &&
      dfrobot_sen0514_prime(&sen0514, sensor.temperature, sensor.humidity/100.0f)) {
    have_calibrated_sen0514 = true;
  }

  if (have_calibrated_sen0514 &&
      dfrobot_sen0514_get_sensor_status(&sen0514, &stat) &&
      /* See the header for an explanation of the status codes */
      stat != DFROBOT_SEN0514_INVALID_OUTPUT) {
# ifdef SNAPPY_READ_CO2
    sensor.have_co2 =
      dfrobot_sen0514_get_co2(&sen0514, &sensor.co2) &&
      sensor.co2 > 400;
    if (sensor.have_co2) {
      LOG("CO2 = %u", sensor.co2);
    }
# endif
# ifdef SNAPPY_READ_VOLATILE_ORGANICS
    sensor.have_tvoc =
      dfrobot_sen0514_get_total_volatile_organic_compounds(&sen0514, &sensor.tvoc) &&
      sensor.tvoc > 0;
    if (sensor.have_tvoc) {
      LOG("TVOC = %u", sensor.tvoc);
    }
# endif
# ifdef SNAPPY_READ_AIR_QUALITY_INDEX
    sensor.have_aqi =
      dfrobot_sen0514_get_air_quality_index(&sen0514, &sensor.aqi) &&
      sensor.aqi >= 1 && sensor.aqi <= 5;
    if (sensor.have_aqi) {
      LOG("AQI = %u", sensor.aqi);
    }
# endif
  }
#endif

#ifdef SNAPPY_READ_MOTION
  /* The motion sensor runs continually while the monitoring window is open, it's interrupt-driven
     or running as a sampler on a separate task.  Motion events are posted back to the main thread
     on the event queue.  */
  sensor.motion = false;
  enable_gpio_sen0171();
#endif

#ifdef SNAPPY_READ_NOISE
  /* The sound sensor runs continually while the monitoring window is open, it is a sampler running
     on a separate task.  Sound level samples are posted back to the main thread on the event
     queue. */
  sound_sampler_start();
#endif

  xTimerStart(monitoring_clock, portMAX_DELAY);
}

void record_motion() {
  sensor.motion = true;
}

void record_noise(uint32_t level) {
  sensor.sound_level = level;
  sensor.have_sound_level = true;
}

void close_monitoring_window() {
#ifdef SNAPPY_READ_MOTION
  disable_gpio_sen0171();
#endif

#ifdef SNAPPY_READ_NOISE
  sound_sampler_stop();
#endif

  LOG("Monitoring window closed");
}
