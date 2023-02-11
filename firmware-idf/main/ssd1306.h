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

/* This is allocated, with a larger buffer, inside a single memory area.  All fields
   should be considered private to the driver.
   
   TODO: This is public only for the sake of SSD1306_DEVICE_SIZE, can we fix that? */
typedef struct {
  unsigned bus;                 /* I2C bus number */
  unsigned addr;                /* I2C device address, unshifted */
  unsigned width;               /* Width of screen in pixels */
  unsigned height;              /* Height of screen in pixels */
  bool     i2c_failure;		/* Set to true if low-level i2c write commands fail */
  bool     initialized;         /* Set to true once ssd1306_Init succeeds */
  bool     display_on;          /* Set to true once the display has been enabled */
} SSD1306_Device_t;

/* This will initialize the device struct.  The only valid heights are 32, 64, and 128.  The effects
   of other values are unspecified. */
bool ssd1306_Create(SSD1306_Device_t* mem, unsigned bus, unsigned i2c_addr,
                    unsigned width, unsigned height) WARN_UNUSED;

/* Procedure definitions.  Where noted these set device->i2c_failure if there's a write failure to
   the device, and will frequently be no-ops if that flag is already set.  Writes outside the buffer
   bounds will silently be ignored.  */

/* Flush the buffer to the OLED device.  Sets device->i2c_failure on failure. */
void ssd1306_UpdateScreen(SSD1306_Device_t* device, framebuf_t* fb);

/* Set the OLED display contrast.  The contrast increases as the value increases.
   The value 7Fh resets the contrast.  Sets device->i2c_failure on failure.  */
void ssd1306_SetContrast(SSD1306_Device_t* device, uint8_t value);

/* Turn the OLED display on or off.  Sets device->i2c_failure on failure.  */
void ssd1306_SetDisplayOn(SSD1306_Device_t* device, bool turn_it_on);

/* Return whether the display is on (true) or off (false) */
bool ssd1306_GetDisplayOn(SSD1306_Device_t* device);

/* ssd1306_WriteI2C() is free within the library; it must be provided by the application.

   Write the bytes to the device, blocking until the write's done. `bus` is the 0-based i2c bus
   number.  `address` is the *unshifted* device address on that bus.  `mem_address` is a memory
   address on the device to which data is written (basically a prefix byte to be sent).  `buffer` is
   the data to write, and `buffer_size` the number of bytes to be written from `buffer`.
   
   If any locking is required for bus access this function will have to perform that locking, as the
   ssd1306 library will not.
  
   Returns true if the write succeeded, false otherwise. */
bool ssd1306_WriteI2C(unsigned bus, unsigned address, unsigned mem_address,
                      uint8_t* buffer, size_t buffer_size) WARN_UNUSED;

#endif /* __SSD1306_H__ */
