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
#include "ssd1306_fonts.h"

#ifdef SSD1306_X_OFFSET
#define SSD1306_X_OFFSET_LOWER (SSD1306_X_OFFSET & 0x0F)
#define SSD1306_X_OFFSET_UPPER ((SSD1306_X_OFFSET >> 4) & 0x07)
#else
#define SSD1306_X_OFFSET_LOWER 0
#define SSD1306_X_OFFSET_UPPER 0
#endif

/* Enumeration for screen colors */
typedef enum {
  SSD1306_BLACK = 0,		/* Black color, no pixel */
  SSD1306_WHITE = 1		/* Pixel is set. Color depends on OLED */
} SSD1306_COLOR;

/* This is allocated, with a larger buffer, inside a single memory area.  All fields
   should be considered private to the driver.
   
   TODO: This is public only for the sake of SSD1306_DEVICE_SIZE, can we fix that? */
typedef struct {
  unsigned bus;                 /* I2C bus number */
  unsigned addr;                /* I2C device address, unshifted */
  unsigned width;               /* Width in pixels */
  unsigned height;              /* Height in pixels */
  unsigned buffer_size;         /* Buffer size in bytes */
  unsigned current_x;           /* Initially (0,0), updated by ssd1306_SetCursor, */
  unsigned current_y;           /*   ssd1306_WriteChar and ssd1306_WriteString */
  bool     i2c_failure;		/* Set to true if low-level i2c write commands fail */
  bool     initialized;         /* Set to true once ssd1306_Init succeeds */
  bool     display_on;          /* Set to true once the display has been enabled */
  uint8_t  buffer[1];		/* Will be larger than this */
} SSD1306_Device_t;

/* Width and height should be constants, in which case this is also constant and can
   be used to allocate memory statically. */
#define SSD1306_DEVICE_SIZE(width, height) (sizeof(SSD1306_Device_t) + (((width) * (height)) / 8) - 1)

/* This will initialize the device struct using the provided memory, which must be large enough to
   hold the struct and the memory for the buffer.

   The only valid heights are 32, 64, and 128.  The effects of other values are unspecified.
*/
SSD1306_Device_t* ssd1306_Create(uint8_t* mem, unsigned bus, unsigned i2c_addr,
                                 unsigned width, unsigned height) WARN_UNUSED;

/* Procedure definitions.  Where noted these set device->i2c_failure if there's a write failure to
   the device, and will frequently be no-ops if that flag is already set.  Writes outside the buffer
   bounds will silently be ignored.  */

/* Initialize the OLED device.

   Mostly for internal use; ssd1306_Create calls it on your behalf.  Sets device->i2c_failure on
   failure.

   NOTE: The caller must wait after bringing up the i2c bus before calling this, typically 100ms. */
void ssd1306_Init(SSD1306_Device_t* device);

/* Fill the buffer with the given color */
void ssd1306_Fill(SSD1306_Device_t* device, SSD1306_COLOR color);

/* Fill the buffer with the given data */
void ssd1306_FillBuffer(SSD1306_Device_t* device, uint8_t* buf, uint32_t len);

/* Flush the buffer to the OLED device.  Sets device->i2c_failure on failure. */
void ssd1306_UpdateScreen(SSD1306_Device_t* device);

/* Set a pixel in the buffer */
void ssd1306_DrawPixel(SSD1306_Device_t* device, uint8_t x, uint8_t y, SSD1306_COLOR color);

/* Set the cursor in the buffer (pixel values) for use with WriteChar and WriteString.  The value
   being set is the top left corner of the character cell.  */
void ssd1306_SetCursor(SSD1306_Device_t* device, uint8_t x, uint8_t y);

/* Write a printable char to the buffer.  Returns true if it was written, otherwise false.  Updates
   the cursor position. */
bool ssd1306_WriteChar(SSD1306_Device_t* device, char ch, FontDef Font, SSD1306_COLOR color);

/* Write a string to the buffer.  Returns a pointer to the first char that could not be written
   (which could be the terminating NUL).  Updates the cursor position.  */
const char* ssd1306_WriteString(SSD1306_Device_t* device, const char* str, FontDef Font,
                                SSD1306_COLOR color);

#ifdef SSD1306_GRAPHICS
/* Draw a line in the buffer */
void ssd1306_Line(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                  SSD1306_COLOR color);

/* Draw an arc in the buffer */
void ssd1306_DrawArc(SSD1306_Device_t* device, uint8_t x, uint8_t y, uint8_t radius,
                     uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color);

/* Draw an arc in the buffer */
void ssd1306_DrawArcWithRadiusLine(SSD1306_Device_t* device, uint8_t x, uint8_t y, uint8_t radius,
                                   uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color);

/* Draw a circle in the buffer */
void ssd1306_DrawCircle(SSD1306_Device_t* device, uint8_t par_x, uint8_t par_y, uint8_t par_r,
                        SSD1306_COLOR color);

/* Draw a filled circle in the buffer */
void ssd1306_FillCircle(SSD1306_Device_t* device, uint8_t par_x,uint8_t par_y,uint8_t par_r,
                        SSD1306_COLOR par_color);

/* Vertex data for Polyline */
typedef struct {
  uint8_t x;
  uint8_t y;
} SSD1306_VERTEX;

/* Draw a set of connected lines in the buffer */
void ssd1306_Polyline(SSD1306_Device_t* device, const SSD1306_VERTEX *par_vertex, uint16_t par_size,
                      SSD1306_COLOR color);

/* Draw a rectangle in the buffer */
void ssd1306_DrawRectangle(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                           SSD1306_COLOR color);

/* Draw a filled rectangle in the buffer */
void ssd1306_FillRectangle(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                           SSD1306_COLOR color);

/* Draw a bitmap in the buffer */
void ssd1306_DrawBitmap(SSD1306_Device_t* device, uint8_t x, uint8_t y,
                        const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);
#endif /* SSD1306_GRAPHICS */

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
