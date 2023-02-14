idf_component_register(
  SRCS

  # Mostly device-independent code - it knows about specific devices but not
  # too many details about them or about how those devices communicate.
  "main.c" "sound_sampler.c" "sound_player.c" "slideshow.c" "sensor.c" "oled.c" "framebuf.c"
  "fonts.c" "bitmaps.c"

  # I2C device driver code that is independent of esp32-idf.
  "dfrobot_sen0500.c" "dfrobot_sen0514.c" "ssd1306.c"

  # Device driver code tied to esp32-idf and the hardware platform.
  "device.c" "dfrobot_sen0487.c" "esp32_ledc_piezo.c" "esp32_i2c.c"

  INCLUDE_DIRS
  ".")
