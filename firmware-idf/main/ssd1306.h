/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for SSD1306-based i2c OLED display
   https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/

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

#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "main.h"
#include "framebuf.h"

/* All fields should be considered private to the driver.  */
typedef struct {
  unsigned bus;                 /* I2C bus number */
  unsigned addr;                /* I2C device address, unshifted */
  unsigned width;               /* Width of screen in pixels */
  unsigned height;              /* Height of screen in pixels */
  unsigned flags;               /* Flag values - internal use */
  bool     i2c_failure;		/* Set to true if low-level i2c write commands fail */
  bool     initialized;         /* Set to true once ssd1306_Init succeeds */
  bool     display_on;          /* Set to true once the display has been enabled */
} SSD1306_Device_t;

enum {
  SSD1306_FLAG_MIRROR_VERT = 1,   /* Mirror vertically */
  SSD1306_FLAG_MIRROR_HORIZ = 2,  /* Mirror horizontally */
  SSD1306_FLAG_INVERSE_COLOR = 4, /* Use inverse colors */
};

/* This will initialize the device struct, then initialize the hardware device.  The I2C bus must
   already have been turned on.  The only valid heights are 32, 64, and 128.  The effects of other
   values are unspecified.  Flags is bitwise OR of SSD1306_FLAG_ values above.  Returns false if
   device initialization failed.

   This will work regardless of the previous contents of `dev` and the current state of the
   display.  */
bool ssd1306_Create(SSD1306_Device_t* dev, unsigned bus, unsigned i2c_addr,
                    unsigned width, unsigned height, unsigned flags) WARN_UNUSED;

/* Device operations.  Where noted these set device->i2c_failure if there's a write failure to the
   device, and will frequently be no-ops if that flag is already set.  Writes outside the buffer
   bounds will silently be ignored.  */

/* Flush the buffer to the OLED device.  If your screen horizontal axis does not start in column 0
   you can use x_offset to adjust the horizontal offset; normally this is zero.  Sets
   device->i2c_failure on failure. */
void ssd1306_UpdateScreen(SSD1306_Device_t* device, framebuf_t* fb, unsigned x_offset);

/* Set the OLED display contrast.  The contrast increases as the value increases.
   The value 7Fh resets the contrast.  Sets device->i2c_failure on failure.  */
void ssd1306_SetContrast(SSD1306_Device_t* device, uint8_t value);

/* Turn the OLED display on or off.  Sets device->i2c_failure on failure.  */
void ssd1306_SetDisplayOn(SSD1306_Device_t* device, bool turn_it_on);

/* Return whether the display is on (true) or off (false) */
bool ssd1306_GetDisplayOn(SSD1306_Device_t* device);

#endif /* __SSD1306_H__ */
