/* -*- fill-column: 100; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Driver for SSD1306-based OLED display
   https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/

   Origin: https://github.com/afiskon/stm32-ssd1306.  Refactored and adapted. */

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

#include "ssd1306.h"
#include "esp32_i2c.h"

static void ssd1306_WriteCommand(SSD1306_Device_t* device, uint8_t byte) {
  if (!device->i2c_failure) {
    if (i2c_mem_write(device->bus, device->addr, 0x00, &byte, 1) != ESP_OK) {
      device->i2c_failure = true;
    }
  }
}

static void ssd1306_WriteData(SSD1306_Device_t* device, uint8_t* buffer, size_t buff_size) {
  if (!device->i2c_failure) {
    if (i2c_mem_write(device->bus, device->addr, 0x40, buffer, buff_size) != ESP_OK) {
      device->i2c_failure = true;
    }
  }
}

static void ssd1306_Init(SSD1306_Device_t* device) {
  /* Init OLED - turn display off */
  ssd1306_SetDisplayOn(device, false);

  /* Set Memory Addressing Mode */
  ssd1306_WriteCommand(device, 0x20);
  /* 00b,Horizontal Addressing Mode
     01b,Vertical Addressing Mode
     10b,Page Addressing Mode (RESET)
     11b,Invalid */
  ssd1306_WriteCommand(device, 0x00);

  /* Set Page Start Address for Page Addressing Mode,0-7 */
  ssd1306_WriteCommand(device, 0xB0);

  if (device->flags & SSD1306_FLAG_MIRROR_VERT) {
    ssd1306_WriteCommand(device, 0xC0); // Mirror vertically
  } else {
    ssd1306_WriteCommand(device, 0xC8); //Set COM Output Scan Direction
  }

  ssd1306_WriteCommand(device, 0x00); //---set low column address
  ssd1306_WriteCommand(device, 0x10); //---set high column address

  ssd1306_WriteCommand(device, 0x40); //--set start line address - CHECK

  ssd1306_SetContrast(device, 0xFF);

  if (device->flags & SSD1306_FLAG_MIRROR_HORIZ) {
    ssd1306_WriteCommand(device, 0xA0); // Mirror horizontally
  } else {
    ssd1306_WriteCommand(device, 0xA1); //--set segment re-map 0 to 127 - CHECK
  }

  if (device->flags & SSD1306_FLAG_INVERSE_COLOR) {
    ssd1306_WriteCommand(device, 0xA7); //--set inverse color
  } else {
    ssd1306_WriteCommand(device, 0xA6); //--set normal color
  }

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
  ssd1306_SetDisplayOn(device, true); //--turn on SSD1306 panel

  device->initialized = true;
}

bool ssd1306_Create(SSD1306_Device_t* device, unsigned bus, unsigned i2c_addr,
                    unsigned width, unsigned height, unsigned flags) {
  device->bus = bus;
  device->addr = i2c_addr;
  device->width = width;
  device->height = height;
  device->flags = flags;
  device->initialized = false;
  device->display_on = false;
  device->i2c_failure = false;
  ssd1306_Init(device);
  return !device->i2c_failure;
}

void ssd1306_UpdateScreen(SSD1306_Device_t* device, framebuf_t* fb) {
  // Write data to each page of RAM. Number of pages
  // depends on the screen height:
  //
  //  * 32px   ==  4 pages
  //  * 64px   ==  8 pages
  //  * 128px  ==  16 pages
  for(uint8_t i = 0; i < device->height/8; i++) {
    ssd1306_WriteCommand(device, 0xB0 + i); // Set the current RAM page address.
    ssd1306_WriteCommand(device, 0x00 /*+ SSD1306_X_OFFSET_LOWER*/);
    ssd1306_WriteCommand(device, 0x10 /*+ SSD1306_X_OFFSET_UPPER*/);
    ssd1306_WriteData(device, &fb->buffer[fb->width*i], device->width);
  }
}

void ssd1306_SetContrast(SSD1306_Device_t* device, uint8_t value) {
  const uint8_t kSetContrastControlRegister = 0x81;
  ssd1306_WriteCommand(device, kSetContrastControlRegister);
  ssd1306_WriteCommand(device, value);
}

void ssd1306_SetDisplayOn(SSD1306_Device_t* device, bool turn_it_on) {
  device->display_on = turn_it_on;
  ssd1306_WriteCommand(device, 0xAE | (int)turn_it_on);
}

bool ssd1306_GetDisplayOn(SSD1306_Device_t* device) {
  return device->display_on;
}
