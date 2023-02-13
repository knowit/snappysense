idf_component_register(SRCS
		       # Mostly device-independent code - it knows about specific devices but not
		       # really too many details about them.
		       "main.c" "sound_sampler.c" "sound_player.c" "slideshow.c" "sensor.c"
		       "oled.c" "framebuf.c" "fonts.c" "bitmaps.c"

  		       # Tightly device-bound code.
		       "device.c" "dfrobot_sen0487.c" "dfrobot_sen0500.c" "dfrobot_sen0514.c"
		       "ssd1306.c" "esp32_ledc_piezo.c" "esp32_i2c.c"
                    INCLUDE_DIRS ".")
