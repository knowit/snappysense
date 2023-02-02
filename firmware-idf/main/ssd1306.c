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

/**
 * This Library was originally written by Olivier Van den Eede (4ilo) in 2016.
 * Some refactoring was done and SPI support was added by Aleksander Alekseev (afiskon) in 2018.
 * Origin: https://github.com/afiskon/stm32-ssd1306
 *
 * SPI support was removed by Lars T Hansen in 2023 and the library was generalized to support
 * multiple I2C devices simultaneously.
 *
 * Dependence on stm32*hal was removed by Lars T Hansen in 2023 in favor of a generic i2c library,
 * see header file.
 */

#include "ssd1306.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

SSD1306_Device_t* ssd1306_i2c_init(uint8_t* mem, unsigned bus, unsigned i2c_addr,
				   unsigned width, unsigned height) {
    SSD1306_Device_t* device = (SSD1306_Device_t*)mem;
    device->bus = bus;
    device->addr = i2c_addr;
    device->width = width;
    device->height = height;
    device->buffer_size = (width * height) / 8;
    device->screen.CurrentX = 0;
    device->screen.CurrentY = 0;
    device->screen.Initialized = 0;
    device->screen.DisplayOn = 0;
    memset(device->buffer, 0, device->buffer_size);
    return device;
}

/* Send a byte to the command register */
void ssd1306_WriteCommand(SSD1306_Device_t* device, uint8_t byte) {
  ssd1306_Write_Blocking(device->bus, device->addr, 0x00, &byte, 1);
}

/* Send data */
void ssd1306_WriteData(SSD1306_Device_t* device, uint8_t* buffer, size_t buff_size) {
  ssd1306_Write_Blocking(device->bus, device->addr, 0x40, buffer, buff_size);
}

/* Fills the Screenbuffer with values from a given buffer of a fixed length */
SSD1306_Error_t ssd1306_FillBuffer(SSD1306_Device_t* device, uint8_t* buf, uint32_t len) {
  SSD1306_Error_t ret = SSD1306_ERR;
  if (len <= device->buffer_size) {
    memcpy(device->buffer,buf,len);
    ret = SSD1306_OK;
  }
  return ret;
}

/* Initialize the oled screen */
void ssd1306_Init(SSD1306_Device_t* device) {
  /* Wait for the screen to boot */
  ssd1306_Delay(100);

  /* Init OLED - turn display off */
  ssd1306_SetDisplayOn(device, 0);

  /* Set Memory Addressing Mode */
  ssd1306_WriteCommand(device, 0x20);
  /* 00b,Horizontal Addressing Mode
     01b,Vertical Addressing Mode
     10b,Page Addressing Mode (RESET)
     11b,Invalid */
  ssd1306_WriteCommand(device, 0x00);

  /* Set Page Start Address for Page Addressing Mode,0-7 */
  ssd1306_WriteCommand(device, 0xB0);

#ifdef SSD1306_MIRROR_VERT
  ssd1306_WriteCommand(device, 0xC0); // Mirror vertically
#else
  ssd1306_WriteCommand(device, 0xC8); //Set COM Output Scan Direction
#endif

  ssd1306_WriteCommand(device, 0x00); //---set low column address
  ssd1306_WriteCommand(device, 0x10); //---set high column address

  ssd1306_WriteCommand(device, 0x40); //--set start line address - CHECK

  ssd1306_SetContrast(device, 0xFF);

#ifdef SSD1306_MIRROR_HORIZ
  ssd1306_WriteCommand(device, 0xA0); // Mirror horizontally
#else
  ssd1306_WriteCommand(device, 0xA1); //--set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
  ssd1306_WriteCommand(device, 0xA7); //--set inverse color
#else
  ssd1306_WriteCommand(device, 0xA6); //--set normal color
#endif

  // Set multiplex ratio.
  if (device->height == 128) {
    // Found in the Luma Python lib for SH1106.
    ssd1306_WriteCommand(device, 0xFF);
  } else {
    ssd1306_WriteCommand(device, 0xA8); //--set multiplex ratio(1 to 64) - CHECK
  }

  if (device->height == 32) {
    ssd1306_WriteCommand(device, 0x1F); //
  } else if (device->height == 64) {
    ssd1306_WriteCommand(device, 0x3F); //
  } else if (device->height == 128) {
    ssd1306_WriteCommand(device, 0x3F); // Seems to work for 128px high displays too.
  } else {
    // TODO: panic("Only 32, 64, or 128 lines of height are supported!")
  }

  ssd1306_WriteCommand(device, 0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content

  ssd1306_WriteCommand(device, 0xD3); //-set display offset - CHECK
  ssd1306_WriteCommand(device, 0x00); //-not offset

  ssd1306_WriteCommand(device, 0xD5); //--set display clock divide ratio/oscillator frequency
  ssd1306_WriteCommand(device, 0xF0); //--set divide ratio

  ssd1306_WriteCommand(device, 0xD9); //--set pre-charge period
  ssd1306_WriteCommand(device, 0x22); //

  ssd1306_WriteCommand(device, 0xDA); //--set com pins hardware configuration - CHECK
  if (device->height == 32) {
    ssd1306_WriteCommand(device, 0x02);
  } else if (device->height == 64) {
    ssd1306_WriteCommand(device, 0x12);
  } else if (device->height == 128) {
    ssd1306_WriteCommand(device, 0x12);
  } else {
    // TODO: panic("Only 32, 64, or 128 lines of height are supported!")
  }

  ssd1306_WriteCommand(device, 0xDB); //--set vcomh
  ssd1306_WriteCommand(device, 0x20); //0x20,0.77xVcc

  ssd1306_WriteCommand(device, 0x8D); //--set DC-DC enable
  ssd1306_WriteCommand(device, 0x14); //
  ssd1306_SetDisplayOn(device, 1); //--turn on SSD1306 panel

  // Clear screen
  ssd1306_Fill(device, SSD1306_BLACK);
    
  // Flush buffer to screen
  ssd1306_UpdateScreen(device);
    
  // Set default values for screen object
  device->screen.CurrentX = 0;
  device->screen.CurrentY = 0;
    
  device->screen.Initialized = 1;
}

/* Fill the whole screen with the given color */
void ssd1306_Fill(SSD1306_Device_t* device, SSD1306_COLOR color) {
  uint32_t i;

  for(i = 0; i < device->buffer_size; i++) {
    device->buffer[i] = (color == SSD1306_BLACK) ? 0x00 : 0xFF;
  }
}

/* Write the screenbuffer with changed to the screen */
void ssd1306_UpdateScreen(SSD1306_Device_t* device) {
  // Write data to each page of RAM. Number of pages
  // depends on the screen height:
  //
  //  * 32px   ==  4 pages
  //  * 64px   ==  8 pages
  //  * 128px  ==  16 pages
  for(uint8_t i = 0; i < device->height/8; i++) {
    ssd1306_WriteCommand(device, 0xB0 + i); // Set the current RAM page address.
    ssd1306_WriteCommand(device, 0x00 + SSD1306_X_OFFSET_LOWER);
    ssd1306_WriteCommand(device, 0x10 + SSD1306_X_OFFSET_UPPER);
    ssd1306_WriteData(device, &device->buffer[device->width*i],device->width);
  }
}

/*
 * Draw one pixel in the screenbuffer
 * X => X Coordinate
 * Y => Y Coordinate
 * color => Pixel color
 */
void ssd1306_DrawPixel(SSD1306_Device_t* device, uint8_t x, uint8_t y, SSD1306_COLOR color) {
  if(x >= device->width || y >= device->height) {
    // Don't write outside the buffer
    return;
  }
   
  // Draw in the right color
  if(color == SSD1306_WHITE) {
    device->buffer[x + (y / 8) * device->width] |= 1 << (y % 8);
  } else { 
    device->buffer[x + (y / 8) * device->width] &= ~(1 << (y % 8));
  }
}

/*
 * Draw 1 char to the screen buffer
 * ch       => char om weg te schrijven
 * Font     => Font waarmee we gaan schrijven
 * color    => Black or White
 */
char ssd1306_WriteChar(SSD1306_Device_t* device, char ch, FontDef Font, SSD1306_COLOR color) {
  uint32_t i, b, j;
    
  // Check if character is valid
  if (ch < 32 || ch > 126)
    return 0;
    
  // Check remaining space on current line
  if (device->width < (device->screen.CurrentX + Font.FontWidth) ||
      device->height < (device->screen.CurrentY + Font.FontHeight))
    {
      // Not enough space on current line
      return 0;
    }
    
  // Use the font to write
  for(i = 0; i < Font.FontHeight; i++) {
    b = Font.data[(ch - 32) * Font.FontHeight + i];
    for(j = 0; j < Font.FontWidth; j++) {
      if((b << j) & 0x8000)  {
	ssd1306_DrawPixel(device, device->screen.CurrentX + j, (device->screen.CurrentY + i),
			  (SSD1306_COLOR) color);
      } else {
	ssd1306_DrawPixel(device, device->screen.CurrentX + j, (device->screen.CurrentY + i),
			  (SSD1306_COLOR)!color);
      }
    }
  }
    
  // The current space is now taken
  device->screen.CurrentX += Font.FontWidth;
    
  // Return written char for validation
  return ch;
}

/* Write full string to screenbuffer */
char ssd1306_WriteString(SSD1306_Device_t* device, char* str, FontDef Font, SSD1306_COLOR color) {
  while (*str) {
    if (ssd1306_WriteChar(device, *str, Font, color) != *str) {
      // Char could not be written
      return *str;
    }
    str++;
  }
    
  // Everything ok
  return *str;
}

/* Position the cursor */
void ssd1306_SetCursor(SSD1306_Device_t* device, uint8_t x, uint8_t y) {
  device->screen.CurrentX = x;
  device->screen.CurrentY = y;
}

#ifdef SSD1306_GRAPHICS
/* Draw line by Bresenham's algorithm */
void ssd1306_Line(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
		  SSD1306_COLOR color) {
  int32_t deltaX = abs(x2 - x1);
  int32_t deltaY = abs(y2 - y1);
  int32_t signX = ((x1 < x2) ? 1 : -1);
  int32_t signY = ((y1 < y2) ? 1 : -1);
  int32_t error = deltaX - deltaY;
  int32_t error2;
    
  ssd1306_DrawPixel(device, x2, y2, color);

  while((x1 != x2) || (y1 != y2)) {
    ssd1306_DrawPixel(device, x1, y1, color);
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

/* Draw polyline */
void ssd1306_Polyline(SSD1306_Device_t* device, const SSD1306_VERTEX *par_vertex, uint16_t par_size,
		      SSD1306_COLOR color) {
  uint16_t i;
  if(par_vertex == NULL) {
    return;
  }

  for(i = 1; i < par_size; i++) {
    ssd1306_Line(device, par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y,
		 color);
  }

  return;
}

/* Convert Degrees to Radians */
static float ssd1306_DegToRad(float par_deg) {
  return par_deg * 3.14 / 180.0;
}

/* Normalize degree to [0;360] */
static uint16_t ssd1306_NormalizeTo0_360(uint16_t par_deg) {
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
void ssd1306_DrawArc(SSD1306_Device_t* device, uint8_t x, uint8_t y,
		     uint8_t radius, uint16_t start_angle, uint16_t sweep, SSD1306_COLOR color) {
  static const uint8_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
  float approx_degree;
  uint32_t approx_segments;
  uint8_t xp1,xp2;
  uint8_t yp1,yp2;
  uint32_t count = 0;
  uint32_t loc_sweep = 0;
  float rad;
    
  loc_sweep = ssd1306_NormalizeTo0_360(sweep);
    
  count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_degree = loc_sweep / (float)approx_segments;
  while(count < approx_segments)
    {
      rad = ssd1306_DegToRad(count*approx_degree);
      xp1 = x + (int8_t)(sin(rad)*radius);
      yp1 = y + (int8_t)(cos(rad)*radius);    
      count++;
      if(count != approx_segments) {
	rad = ssd1306_DegToRad(count*approx_degree);
      } else {
	rad = ssd1306_DegToRad(loc_sweep);
      }
      xp2 = x + (int8_t)(sin(rad)*radius);
      yp2 = y + (int8_t)(cos(rad)*radius);    
      ssd1306_Line(device,xp1,yp1,xp2,yp2,color);
    }
    
  return;
}

/*
 * Draw arc with radius line
 * Angle is beginning from 4 quart of trigonometric circle (3pi/2)
 * start_angle: start angle in degree
 * sweep: finish angle in degree
 */
void ssd1306_DrawArcWithRadiusLine(SSD1306_Device_t* device, uint8_t x, uint8_t y,
				   uint8_t radius, uint16_t start_angle, uint16_t sweep,
				   SSD1306_COLOR color) {
  static const uint8_t CIRCLE_APPROXIMATION_SEGMENTS = 36;
  float approx_degree;
  uint32_t approx_segments;
  uint8_t xp1 = 0;
  uint8_t xp2 = 0;
  uint8_t yp1 = 0;
  uint8_t yp2 = 0;
  uint32_t count = 0;
  uint32_t loc_sweep = 0;
  float rad;
    
  loc_sweep = ssd1306_NormalizeTo0_360(sweep);
    
  count = (ssd1306_NormalizeTo0_360(start_angle) * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_segments = (loc_sweep * CIRCLE_APPROXIMATION_SEGMENTS) / 360;
  approx_degree = loc_sweep / (float)approx_segments;

  rad = ssd1306_DegToRad(count*approx_degree);
  uint8_t first_point_x = x + (int8_t)(sin(rad)*radius);
  uint8_t first_point_y = y + (int8_t)(cos(rad)*radius);   
  while (count < approx_segments) {
    rad = ssd1306_DegToRad(count*approx_degree);
    xp1 = x + (int8_t)(sin(rad)*radius);
    yp1 = y + (int8_t)(cos(rad)*radius);    
    count++;
    if (count != approx_segments) {
      rad = ssd1306_DegToRad(count*approx_degree);
    } else {
      rad = ssd1306_DegToRad(loc_sweep);
    }
    xp2 = x + (int8_t)(sin(rad)*radius);
    yp2 = y + (int8_t)(cos(rad)*radius);    
    ssd1306_Line(device,xp1,yp1,xp2,yp2,color);
  }
    
  // Radius line
  ssd1306_Line(device,x,y,first_point_x,first_point_y,color);
  ssd1306_Line(device,x,y,xp2,yp2,color);
  return;
}

/* Draw circle by Bresenham's algorithm */
void ssd1306_DrawCircle(SSD1306_Device_t* device, uint8_t par_x, uint8_t par_y, uint8_t par_r,
			SSD1306_COLOR par_color) {
  int32_t x = -par_r;
  int32_t y = 0;
  int32_t err = 2 - 2 * par_r;
  int32_t e2;

  if (par_x >= device->width || par_y >= device->height) {
    return;
  }

  do {
    ssd1306_DrawPixel(device, par_x - x, par_y + y, par_color);
    ssd1306_DrawPixel(device, par_x + x, par_y + y, par_color);
    ssd1306_DrawPixel(device, par_x + x, par_y - y, par_color);
    ssd1306_DrawPixel(device, par_x - x, par_y - y, par_color);
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
void ssd1306_FillCircle(SSD1306_Device_t* device, uint8_t par_x,uint8_t par_y,uint8_t par_r,
			SSD1306_COLOR par_color) {
  int32_t x = -par_r;
  int32_t y = 0;
  int32_t err = 2 - 2 * par_r;
  int32_t e2;

  if (par_x >= device->width || par_y >= device->height) {
    return;
  }

  do {
    for (uint8_t _y = (par_y + y); _y >= (par_y - y); _y--) {
      for (uint8_t _x = (par_x - x); _x >= (par_x + x); _x--) {
	ssd1306_DrawPixel(device, _x, _y, par_color);
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
void ssd1306_DrawRectangle(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
			   SSD1306_COLOR color) {
  ssd1306_Line(device,x1,y1,x2,y1,color);
  ssd1306_Line(device,x2,y1,x2,y2,color);
  ssd1306_Line(device,x2,y2,x1,y2,color);
  ssd1306_Line(device,x1,y2,x1,y1,color);

  return;
}

/* Draw a filled rectangle */
void ssd1306_FillRectangle(SSD1306_Device_t* device, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
			   SSD1306_COLOR color) {
  uint8_t x_start = ((x1<=x2) ? x1 : x2);
  uint8_t x_end   = ((x1<=x2) ? x2 : x1);
  uint8_t y_start = ((y1<=y2) ? y1 : y2);
  uint8_t y_end   = ((y1<=y2) ? y2 : y1);

  for (uint8_t y= y_start; (y<= y_end)&&(y<device->height); y++) {
    for (uint8_t x= x_start; (x<= x_end)&&(x<device->width); x++) {
      ssd1306_DrawPixel(device, x, y, color);
    }
  }
  return;
}

/* Draw a bitmap */
void ssd1306_DrawBitmap(SSD1306_Device_t* device, uint8_t x, uint8_t y,
			const unsigned char* bitmap, uint8_t w, uint8_t h, SSD1306_COLOR color) {
  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;

  if (x >= device->width || y >= device->height) {
    return;
  }

  for (uint8_t j = 0; j < h; j++, y++) {
    for (uint8_t i = 0; i < w; i++) {
      if (i & 7) {
	byte <<= 1;
      } else {
	byte = (*(const unsigned char *)(&bitmap[j * byteWidth + i / 8]));
      }

      if (byte & 0x80) {
	ssd1306_DrawPixel(device, x + i, y, color);
      }
    }
  }
  return;
}
#endif // SSD1306_GRAPHICS

void ssd1306_SetContrast(SSD1306_Device_t* device, const uint8_t value) {
  const uint8_t kSetContrastControlRegister = 0x81;
  ssd1306_WriteCommand(device, kSetContrastControlRegister);
  ssd1306_WriteCommand(device, value);
}

void ssd1306_SetDisplayOn(SSD1306_Device_t* device, const uint8_t on) {
  uint8_t value;
  if (on) {
    value = 0xAF;   // Display on
    device->screen.DisplayOn = 1;
  } else {
    value = 0xAE;   // Display off
    device->screen.DisplayOn = 0;
  }
  ssd1306_WriteCommand(device, value);
}

uint8_t ssd1306_GetDisplayOn(SSD1306_Device_t* device) {
  return device->screen.DisplayOn;
}
