/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#include "oled.h"
#include "device.h"
#include "framebuf.h"
#include "bitmaps.h"
#include <stdarg.h>

#ifdef SNAPPY_OLED

/* Screen state */
# ifdef SNAPPY_I2C_SSD1306
#  define FB_WIDTH SSD1306_WIDTH
#  define FB_HEIGHT SSD1306_HEIGHT
# else
#  define FB_WIDTH 100
#  define FB_HEIGHT 100
# endif

static uint8_t bufmem[(FB_WIDTH*FB_HEIGHT+7)/8];
static framebuf_t fb = {
  .width = FB_WIDTH,
  .height = FB_HEIGHT,
  .buffer_size = sizeof(bufmem),
  .current_x = 0,
  .current_y = 0,
  .buffer = bufmem
};

bool oled_present() {
# ifdef SNAPPY_I2C_SSD1306
  return have_ssd1306;
# else
  return false;
#endif
}

void oled_clear_screen() {
  fb_fill(&fb, fb_black);
# ifdef SNAPPY_I2C_SSD1306
  if (have_ssd1306) {
    ssd1306_UpdateScreen(&ssd1306, &fb, 0);
  }
# endif
}

void oled_splash_screen() {
  fb_fill(&fb, fb_black);
  fb_draw_bitmap(&fb, 0, 1,
		 knowit_logo_bitmap, KNOWIT_LOGO_WIDTH, KNOWIT_LOGO_HEIGHT,
		 fb_white);
# ifdef SNAPPY_I2C_SSD1306
  if (have_ssd1306) {
    ssd1306_UpdateScreen(&ssd1306, &fb, 0);
  }
#endif
}

void oled_show_text(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buf[32*4];		/* Largest useful string for this screen and font */
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  fb_fill(&fb, fb_black);
  char* p = buf;
  int y = 0;
  while (*p) {
    char* start = p;
    while (*p && *p != '\n') {
      p++;
    }
    if (*p) {
      *p++ = 0;
    }
    fb_set_cursor(&fb, 0, y);
    fb_write_string(&fb, start, Font_7x10, fb_white);
    y += Font_7x10.FontHeight + 1;
  }
# ifdef SNAPPY_I2C_SSD1306
  if (have_ssd1306) {
    ssd1306_UpdateScreen(&ssd1306, &fb, 0);
  }
# endif
}
#endif
