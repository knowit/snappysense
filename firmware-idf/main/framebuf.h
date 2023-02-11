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

#ifndef framebuf_h_included
#define framebuf_h_included

#include "main.h"
#include "fonts.h"

typedef struct framebuf {
  unsigned width;               /* Width of buffer in pixels */
  unsigned height;              /* Height of buffer in pixels */
  unsigned buffer_size;         /* Buffer size in bytes, must be ceil(width*height/8) bytes */
  unsigned current_x;           /* Initially (0,0), updated by fb_set_cursor, */
  unsigned current_y;           /*   fb_write_char and fb_write_string */
  uint8_t* buffer;		/* Buffer must be buffer_size bytes */
} framebuf_t;

typedef enum {
  fb_black = 0,
  fb_white = 1
} fb_color_t;

/* Fill the buffer with the given color */
void fb_fill(framebuf_t* fb, fb_color_t color);

/* Fill the buffer with the given data (basically a memcpy) */
void fb_fill_buffer(framebuf_t* fb, uint8_t* buf, unsigned len);

/* Set a pixel in the buffer */
void fb_draw_pixel(framebuf_t* fb, unsigned x, unsigned y, fb_color_t color);

/* Set the cursor in the buffer (pixel values) for use with WriteChar and WriteString.  The value
   being set is the top left corner of the character cell.  */
void fb_set_cursor(framebuf_t* fb, unsigned x, unsigned y);

/* Write a printable char to the buffer.  Returns true if it was written, otherwise false.  Updates
   the cursor position. */
bool fb_write_char(framebuf_t* fb, char ch, FontDef Font, fb_color_t color);

/* Write a string to the buffer.  Returns a pointer to the first char that could not be written
   (which could be the terminating NUL).  Updates the cursor position.  */
const char* fb_write_string(framebuf_t* fb, const char* str, FontDef Font, fb_color_t color);

#ifdef FRAMEBUFFER_GRAPHICS

/* Draw a line in the buffer */
void fb_line(framebuf_t* fb, unsigned x1, unsigned y1, unsigned x2, unsigned y2, fb_color_t color);

/* Draw an arc in the buffer */
void fb_draw_arc(framebuf_t* fb, unsigned x, unsigned y, unsigned radius, unsigned start_angle,
		 unsigned sweep, fb_color_t color);

/* Draw an arc in the buffer */
void fb_draw_arc_with_radius_line(framebuf_t* fb, unsigned x, unsigned y, unsigned radius,
				  unsigned start_angle, unsigned sweep, fb_color_t color);

/* Draw a circle in the buffer */
void fb_draw_circle(framebuf_t* fb, unsigned par_x, unsigned par_y, unsigned par_r,
		    fb_color_t color);

/* Draw a filled circle in the buffer */
void fb_fill_circle(framebuf_t* fb, unsigned par_x, unsigned par_y, unsigned par_r,
		    fb_color_t par_color);

/* Vertex data for Polyline */
typedef struct {
  unsigned x;
  unsigned y;
} fb_vertex;

/* Draw a set of connected lines in the buffer */
void fb_polyline(framebuf_t* fb, const fb_vertex *par_vertex, unsigned par_size, fb_color_t color);

/* Draw a rectangle in the buffer */
void fb_draw_rectangle(framebuf_t* fb, unsigned x1, unsigned y1, unsigned x2, unsigned y2,
		       fb_color_t color);

/* Draw a filled rectangle in the buffer */
void fb_fill_rectangle(framebuf_t* fb, unsigned x1, unsigned y1, unsigned x2, unsigned y2,
		       fb_color_t color);

/* Draw a bitmap in the buffer */
void fb_draw_bitmap(framebuf_t* fb, unsigned x, unsigned y, const unsigned char* bitmap,
		    unsigned w, unsigned h, fb_color_t color);

#endif /* FRAMEBUFFER_GRAPHICS */

#endif /* !framebuf_h_included */
