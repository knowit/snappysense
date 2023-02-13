idf_component_register(SRCS # Mostly device-independent code
			    "main.c" "sound_sampler.c" "sound_player.c" "framebuf.c" "fonts.c" "bitmaps.c"
			    # Devices
			    "device.c" "dfrobot_sen0487.c" "dfrobot_sen0500.c" "dfrobot_sen0514.c" "ssd1306.c"
			    "piezo.c"
                    INCLUDE_DIRS ".")
