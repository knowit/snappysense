/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Basic monochrome framebuffer code.
   Origin: https://github.com/afiskon/stm32-ssd1306.  Extensively modified.  */

/*
MIT License

Copyright (c) 2018 Aleksander Alekseev

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "framebuf.h"
#include <string.h>
#include <math.h>

void fb_fill(framebuf_t* fb, fb_color_t color) {
  uint32_t i;

  for(i = 0; i < fb->buffer_size; i++) {
    fb->buffer[i] = (color == fb_black) ? 0x00 : 0xFF;
  }
}

void fb_fill_buffer(framebuf_t* fb, uint8_t* buf, unsigned len) {
  if (len <= fb->buffer_size) {
    memcpy(fb->buffer, buf, len);
  }
}

void fb_draw_pixel(framebuf_t* fb, unsigned x, unsigned y, fb_color_t color) {
  if(x >= fb->width || y >= fb->height) {
    // Don't write outside the buffer
    return;
  }
   
  // Draw in the right color
  if(color == fb_white) {
    fb->buffer[x + (y / 8) * fb->width] |= 1 << (y % 8);
  } else { 
    fb->buffer[x + (y / 8) * fb->width] &= ~(1 << (y % 8));
  }
}


/* Draw a bitmap */
void fb_draw_bitmap(framebuf_t* fb, unsigned x, unsigned y,
			const unsigned char* bitmap, unsigned w, unsigned h, fb_color_t color) {
  int byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  if (x >= fb->width || y >= fb->height) {
    return;
  }

  for (unsigned j = 0; j < h; j++, y++) {
    for (unsigned i = 0; i < w; i++) {
      if (i & 7) {
	byte <<= 1;
      } else {
	byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
      }

      if (byte & 0x80) {
	fb_draw_pixel(fb, x + i, y, color);
      }
    }
  }
  return;
}

bool fb_write_char(framebuf_t* fb, char ch, FontDef Font, fb_color_t color) {
  uint32_t i, b, j;
    
  // Check if character is valid
  if (ch < 32 || ch > 126)
    return false;
    
  // Check remaining space on current line
  if (fb->width < (fb->current_x + Font.FontWidth) ||
      fb->height < (fb->current_y + Font.FontHeight))
    {
      // Not enough space on current line
      return false;
    }
    
  // Use the font to write
  for(i = 0; i < Font.FontHeight; i++) {
    b = Font.data[(ch - 32) * Font.FontHeight + i];
    for(j = 0; j < Font.FontWidth; j++) {
      if((b << j) & 0x8000)  {
	fb_draw_pixel(fb, fb->current_x + j, (fb->current_y + i), color);
      } else {
	fb_draw_pixel(fb, fb->current_x + j, (fb->current_y + i), color ^ 1);
      }
    }
  }
    
  // The current space is now taken
  fb->current_x += Font.FontWidth;
  return true;
}

const char* fb_write_string(framebuf_t* fb, const char* str, FontDef Font, fb_color_t color) {
  while (*str && fb_write_char(fb, *str, Font, color)) {
    str++;
  }
  return str;
}

void fb_set_cursor(framebuf_t* fb, unsigned x, unsigned y) {
  fb->current_x = x;
  fb->current_y = y;
}

#ifdef FRAMEBUFFER_GRAPHICS
/* Draw line by Bresenham's algorithm */
void fb_line(framebuf_t* fb, unsigned x1, unsigned y1, unsigned x2, unsigned y2, fb_color_t color) {
  int deltaX = abs((int)x2 - (int)x1);
  int deltaY = abs((int)y2 - (int)y1);
  int signX = ((x1 < x2) ? 1 : -1);
  int signY = ((y1 < y2) ? 1 : -1);
  int error = deltaX - deltaY;
  int error2;
    
  fb_draw_pixel(fb, x2, y2, color);

  while((x1 != x2) || (y1 != y2)) {
    fb_draw_pixel(fb, x1, y1, color);
    error2 = error * 2;
    if(error2 > -deltaY) {
      error -= deltaY;
      x1 += signX;
    }
        
    if(error2 < deltaX) {
      error += deltaX;
      y1 += signY;
    }
  }
  return;
}

void fb_polyline(framebuf_t* fb, const fb_vertex *par_vertex, unsigned par_size, fb_color_t color) {
  uint16_t i;
  if(par_vertex == NULL) {
    return;
  }

  for(i = 1; i < par_size; i++) {
    fb_line(fb, par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
  }

  return;
}

static float deg_to_rad(float par_deg) {
  return par_deg * 3.14 / 180.0;
}

static uint16_t normalize_to_0_360(uint16_t par_deg) {
  uint16_t loc_angle;
  if(par_deg <= 360) {
    loc_angle = par_deg;
  } else {
    loc_angle = par_deg % 360;
    loc_angle = ((par_deg != 0)?par_deg:360);
  }
  return loc_angle;
}

/*
 * DrawArc. Draw angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle in degree
 * sweep in degree
 */
void fb_draw_arc(framebuf_t* fb, unsigned x, unsigned y,
		 unsigned radius, unsigned start_angle, unsigned sweep, fb_color_t color) {
  static const unsigned CIRCLE_APPROXIMATION_SEGMENTS = 36;
  float approx_degree;
  unsigned approx_segments;
  unsigned xp1, xp2;
  unsigned yp1, yp2;
  unsigned count = 0;
  unsigned loc_sweep = 0;
  float rad;
    
  loc_sweep = normalize_to_0_360(sweep);
    
  count = (normalize_to_0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_degree = loc_sweep / (float)approx_segments;
  while(count < approx_segments)
    {
      rad = deg_to_rad(count*approx_degree);
      xp1 = x + (int)(sin(rad)*radius);
      yp1 = y + (int)(cos(rad)*radius);    
      count++;
      if(count != approx_segments) {
	rad = deg_to_rad(count*approx_degree);
      } else {
	rad = deg_to_rad(loc_sweep);
      }
      xp2 = x + (int)(sin(rad)*radius);
      yp2 = y + (int)(cos(rad)*radius);    
      fb_line(fb,xp1,yp1,xp2,yp2,color);
    }
    
  return;
}

/*
 * Draw arc with radius line
 * Angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle: start angle in degree
 * sweep: finish angle in degree
 */
void fb_draw_arc_with_radius_line(framebuf_t* fb, unsigned x, unsigned y,
				   unsigned radius, unsigned start_angle, unsigned sweep,
				   fb_color_t color) {
  static const unsigned CIRCLE_APPROXIMATION_SEGMENTS = 36;
  float approx_degree;
  unsigned approx_segments;
  unsigned xp1 = 0;
  unsigned xp2 = 0;
  unsigned yp1 = 0;
  unsigned yp2 = 0;
  unsigned count = 0;
  unsigned loc_sweep = 0;
  float rad;
    
  loc_sweep = normalize_to_0_360(sweep);
    
  count = (normalize_to_0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_degree = loc_sweep / (float)approx_segments;

  rad = deg_to_rad(count*approx_degree);
  unsigned first_point_x = x + (int)(sin(rad)*radius);
  unsigned first_point_y = y + (int)(cos(rad)*radius);   
  while (count < approx_segments) {
    rad = deg_to_rad(count*approx_degree);
    xp1 = x + (int)(sin(rad)*radius);
    yp1 = y + (int)(cos(rad)*radius);    
    count++;
    if (count != approx_segments) {
      rad = deg_to_rad(count*approx_degree);
    } else {
      rad = deg_to_rad(loc_sweep);
    }
    xp2 = x + (int)(sin(rad)*radius);
    yp2 = y + (int)(cos(rad)*radius);    
    fb_line(fb,xp1,yp1,xp2,yp2,color);
  }
    
  // Radius line
  fb_line(fb,x,y,first_point_x,first_point_y,color);
  fb_line(fb,x,y,xp2,yp2,color);
  return;
}

/* Draw circle by Bresenham's algorithm */
void fb_draw_circle(framebuf_t* fb, unsigned par_x, unsigned par_y, unsigned par_r,
		    fb_color_t par_color) {
  int x = -par_r;
  int y = 0;
  int err = 2 - 2 * par_r;
  int e2;

  if (par_x >= fb->width || par_y >= fb->height) {
    return;
  }

  do {
    fb_draw_pixel(fb, par_x - x, par_y + y, par_color);
    fb_draw_pixel(fb, par_x + x, par_y + y, par_color);
    fb_draw_pixel(fb, par_x + x, par_y - y, par_color);
    fb_draw_pixel(fb, par_x - x, par_y - y, par_color);
    e2 = err;

    if (e2 <= y) {
      y++;
      err = err + (y * 2 + 1);
      if(-x == y && e2 <= x) {
	e2 = 0;
      }
    }

    if (e2 > x) {
      x++;
      err = err + (x * 2 + 1);
    }
  } while (x <= 0);

  return;
}

/* Draw filled circle. Pixel positions calculated using Bresenham's algorithm */
void fb_fill_circle(framebuf_t* fb, unsigned par_x, unsigned par_y, unsigned par_r,
		    fb_color_t par_color) {
  int x = -par_r;
  int y = 0;
  int err = 2 - 2 * par_r;
  int e2;

  if (par_x >= fb->width || par_y >= fb->height) {
    return;
  }

  do {
    for (unsigned _y = (par_y + y); _y >= (par_y - y); _y--) {
      for (unsigned _x = (par_x - x); _x >= (par_x + x); _x--) {
	fb_draw_pixel(fb, _x, _y, par_color);
      }
    }

    e2 = err;
    if (e2 <= y) {
      y++;
      err = err + (y * 2 + 1);
      if (-x == y && e2 <= x) {
	e2 = 0;
      }
    }

    if (e2 > x) {
      x++;
      err = err + (x * 2 + 1);
    }
  } while (x <= 0);

  return;
}

/* Draw a rectangle */
void fb_draw_rectangle(framebuf_t* fb, unsigned x1, unsigned y1, unsigned x2, unsigned y2,
			   fb_color_t color) {
  fb_line(fb,x1,y1,x2,y1,color);
  fb_line(fb,x2,y1,x2,y2,color);
  fb_line(fb,x2,y2,x1,y2,color);
  fb_line(fb,x1,y2,x1,y1,color);

  return;
}

/* Draw a filled rectangle */
void fb_fill_rectangle(framebuf_t* fb, unsigned x1, unsigned y1, unsigned x2, unsigned y2,
			   fb_color_t color) {
  unsigned x_start = ((x1<=x2) ? x1 : x2);
  unsigned x_end   = ((x1<=x2) ? x2 : x1);
  unsigned y_start = ((y1<=y2) ? y1 : y2);
  unsigned y_end   = ((y1<=y2) ? y2 : y1);

  for (unsigned y= y_start; (y<= y_end)&&(y<fb->height); y++) {
    for (unsigned x= x_start; (x<= x_end)&&(x<fb->width); x++) {
      fb_draw_pixel(fb, x, y, color);
    }
  }
  return;
}
#endif // FRAMEBUFFER_GRAPHICS
