/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for SSD1306-based OLED display
   https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/ */

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

/* Origin: https://github.com/afiskon/stm32-ssd1306
 
   This Library was originally written by Olivier Van den Eede (4ilo) in 2016.
   Some refactoring was done and SPI support was added by Aleksander Alekseev (afiskon) in 2018.
  
   SPI support was removed by Lars T Hansen in 2023 and the library was generalized to support
   multiple I2C devices (on separate busses, since the device address is hardcoded) simultaneously.
 
   Dependence on stm32*hal was removed by Lars T Hansen in 2023 in favor of a generic i2c library,
   see the bottom of this file for signatures of free functions that must be provided.  This code is
   free of dependencies on specific platforms (apart from needing to be able to do blocking delays
   and blocking i2c writes). */

#ifndef __SSD1306_H__
#define __SSD1306_H__

#include <_ansi.h>

_BEGIN_STD_C

#include "ssd1306_conf.h"

#ifdef SSD1306_X_OFFSET
#define SSD1306_X_OFFSET_LOWER (SSD1306_X_OFFSET & 0x0F)
#define SSD1306_X_OFFSET_UPPER ((SSD1306_X_OFFSET >> 4) & 0x07)
#else
#define SSD1306_X_OFFSET_LOWER 0
#define SSD1306_X_OFFSET_UPPER 0
#endif

#include "ssd1306_fonts.h"

/* Enumeration for screen colors */
typedef enum {
  SSD1306_BLACK = 0,		/* Black color, no pixel */
  SSD1306_WHITE = 1		/* Pixel is set. Color depends on OLED */
} SSD1306_COLOR;

typedef enum {
    SSD1306_OK = 0x00,
    SSD1306_ERR = 0x01  /* Generic error. */
} SSD1306_Error_t;

/* Struct to store transformations */
typedef struct {
  uint16_t CurrentX;
  uint16_t CurrentY;
  uint8_t Initialized;
  uint8_t DisplayOn;
} SSD1306_t;

typedef struct {
  unsigned bus;
  unsigned addr;
  unsigned width;
  unsigned height;
  unsigned buffer_size;
  bool i2c_failure;		/* Set to true if low-level i2c write commands fail */
  SSD1306_t screen;
  uint8_t buffer[1];		/* Will be larger */
} SSD1306_Device_t;

/* Width and height should be constants, in which case this is also constant and can
   be used to allocate memory statically. */
#define SSD1306_DEVICE_SIZE(width, height) (sizeof(SSD1306_Device_t) + (((width) * (height)) / 8) - 1)

/* This will initialize the device struct using the provided memory, which must be large enough to
   hold the struct and the memory for the buffer. */
SSD1306_Device_t* ssd1306_Create(uint8_t* mem, unsigned bus, unsigned i2c_addr, unsigned width, unsigned height);

typedef struct {
  uint8_t x;
  uint8_t y;
} SSD1306_VERTEX;

/* Procedure definitions */
void ssd1306_Init(SSD1306_Device_t* device);
void ssd1306_Fill(SSD1306_Device_t* device, SSD1306_COLOR color);
void ssd1306_UpdateScreen(SSD1306_Device_t* device);
void ssd1306_DrawPixel(SSD1306_Device_t* device, uint8_t x, uint8_t y, SSD1306_COLOR color);
char ssd1306_WriteChar(SSD1306_Device_t* device, char ch, FontDef Font, SSD1306_COLOR color);
char ssd1306_WriteString(SSD1306_Device_t* device, char* str, FontDef Font, SSD1306_COLOR color);
void ssd1306_SetCursor(SSD1306_Device_t* device, uint8_t x, uint8_t y);
#ifdef SSD1306_GRAPHICS
void ssd1306_Line(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawArc(SSD1306_Device_t* device, uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color);
void ssd1306_DrawArcWithRadiusLine(SSD1306_Device_t* device, uint8_t x, uint8_t y, uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color);
void ssd1306_DrawCircle(SSD1306_Device_t* device, uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR color);
void ssd1306_FillCircle(SSD1306_Device_t* device, uint8_t par_x,uint8_t par_y,uint8_t par_r,SSD1306_COLOR par_color);
void ssd1306_Polyline(SSD1306_Device_t* device, const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color);
void ssd1306_DrawRectangle(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_FillRectangle(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);
void ssd1306_DrawBitmap(SSD1306_Device_t* device, uint8_t x, uint8_t y, const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color);
#endif

/**
 * @brief Sets the contrast of the display.
 * @param[in] value contrast to set.
 * @note Contrast increases as the value increases.
 * @note RESET = 7Fh.
 */
void ssd1306_SetContrast(SSD1306_Device_t* device, const uint8_t value);

/**
 * @brief Set Display ON/OFF.
 * @param[in] on 0 for OFF, any for ON.
 */
void ssd1306_SetDisplayOn(SSD1306_Device_t* device, const uint8_t on);

/**
 * @brief Reads DisplayOn state.
 * @return  0: OFF.
 *          1: ON.
 */
uint8_t ssd1306_GetDisplayOn(SSD1306_Device_t* device);

/* Low-level procedures */
bool ssd1306_WriteCommand(SSD1306_Device_t* device, uint8_t byte);
bool ssd1306_WriteData(SSD1306_Device_t* device, uint8_t* buffer, size_t buff_size);
SSD1306_Error_t ssd1306_FillBuffer(SSD1306_Device_t* device, uint8_t* buf, uint32_t len);


/* This function is free within the library; it must be provided by the application. */

/* Write the bytes to the device, blocking until the write's done. `bus` is the 0-based i2c bus
   number.  `address` is the *unshifted* device address on that bus.  `mem_address` is a memory
   address on the device to which data is written.  `buffer` is the data to write, and `buffer_size`
   the number of bytes to be written from `buffer`.
   
   If any locking is required for bus access this function will have to perform that locking, as the
   ssd1306 library will not.
  
   Returns true if the write succeeded, false otherwise. */
bool ssd1306_Write_Blocking(unsigned bus, unsigned address, unsigned mem_address,
			    uint8_t* buffer, size_t buffer_size);

_END_STD_C

#endif /* __SSD1306_H__ */
