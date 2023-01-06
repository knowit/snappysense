// Icons for the device display

// Icon data can be created by using the tool at:
// http://en.radzio.dxp.pl/bitmap_converter/
// (select a monochrome bitmap, upto 128x32 pixels in size, horizontal style)

#include "icons.h"

const unsigned char knowit_logo [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00,
0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x1E, 0x00,
0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x1E, 0x00,
0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x1E, 0x00,
0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00,
0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1E, 0x00,
0x3C, 0x00, 0x3C, 0xFC, 0x7E, 0x00, 0x01, 0xF8, 0x07, 0xF8, 0x0C, 0x07, 0xF9, 0xF0, 0x7F, 0xF0,
0x3C, 0x00, 0xFC, 0xFD, 0xFF, 0x80, 0x0F, 0xFF, 0x0F, 0xF8, 0x1E, 0x0F, 0xF9, 0xF8, 0xFF, 0xF8,
0x3C, 0x01, 0xFC, 0xFF, 0xFF, 0xE0, 0x1F, 0xFF, 0x8F, 0xF8, 0x3E, 0x0F, 0xF9, 0xF8, 0xFF, 0xF8,
0x3C, 0x03, 0xFC, 0xFF, 0xFF, 0xE0, 0x3F, 0xFF, 0xCF, 0xF8, 0x3E, 0x0F, 0xF9, 0xF8, 0xFF, 0xF8,
0x3C, 0x07, 0xC0, 0x3F, 0x81, 0xF0, 0x7E, 0x07, 0xE1, 0xE0, 0x3F, 0x01, 0xC0, 0x78, 0x1E, 0x00,
0x3C, 0x0F, 0x80, 0x3E, 0x00, 0xF0, 0x78, 0x01, 0xF1, 0xE0, 0x3F, 0x03, 0xC0, 0x78, 0x1E, 0x00,
0x3C, 0x1F, 0x00, 0x3E, 0x00, 0xF0, 0xF0, 0x01, 0xF0, 0xE0, 0x7F, 0x03, 0xC0, 0x78, 0x1E, 0x00,
0x3C, 0x3E, 0x00, 0x3C, 0x00, 0x78, 0xF0, 0x00, 0xF0, 0xF0, 0x7F, 0x83, 0xC0, 0x78, 0x1E, 0x00,
0x3C, 0x7C, 0x00, 0x3C, 0x00, 0x79, 0xE0, 0x00, 0x78, 0xF0, 0x77, 0x87, 0x80, 0x78, 0x1E, 0x00,
0x3C, 0xF8, 0x00, 0x3C, 0x00, 0x79, 0xE0, 0x00, 0x78, 0xF0, 0xF7, 0x87, 0x80, 0x78, 0x1E, 0x00,
0x3D, 0xF8, 0x00, 0x3C, 0x00, 0x79, 0xE0, 0x00, 0x78, 0x78, 0xF3, 0xC7, 0x80, 0x78, 0x1E, 0x00,
0x3F, 0xFC, 0x00, 0x3C, 0x00, 0x79, 0xE0, 0x00, 0x78, 0x79, 0xE3, 0xCF, 0x00, 0x78, 0x1E, 0x0F,
0x3F, 0xFC, 0x00, 0x3C, 0x00, 0x79, 0xE0, 0x00, 0x78, 0x79, 0xE1, 0xCF, 0x00, 0x78, 0x1E, 0x0F,
0x3F, 0x9E, 0x00, 0x3C, 0x00, 0x79, 0xE0, 0x00, 0x78, 0x3D, 0xC1, 0xEE, 0x00, 0x78, 0x1E, 0x0F,
0x3F, 0x1F, 0x00, 0x3C, 0x00, 0x78, 0xF0, 0x00, 0xF0, 0x3F, 0xC1, 0xFE, 0x00, 0x78, 0x1E, 0x0F,
0x3E, 0x0F, 0x80, 0x3C, 0x00, 0x78, 0xF8, 0x01, 0xF0, 0x1F, 0xC0, 0xFE, 0x00, 0x78, 0x1E, 0x0F,
0x3C, 0x07, 0xC0, 0x3C, 0x00, 0x78, 0x78, 0x01, 0xF0, 0x1F, 0xC0, 0xFC, 0x00, 0x78, 0x1E, 0x0F,
0x3C, 0x03, 0xE0, 0x3C, 0x00, 0x78, 0x7E, 0x07, 0xE0, 0x1F, 0x80, 0xFC, 0x00, 0x78, 0x1F, 0x0F,
0xFF, 0x81, 0xFC, 0xFF, 0x81, 0xFE, 0x3F, 0xFF, 0xC0, 0x0F, 0x80, 0x7C, 0x07, 0xFE, 0x0F, 0xFE,
0xFF, 0x81, 0xFC, 0xFF, 0x81, 0xFE, 0x1F, 0xFF, 0x80, 0x0F, 0x80, 0x7C, 0x07, 0xFE, 0x0F, 0xFE,
0xFF, 0x80, 0xFC, 0xFF, 0x81, 0xFE, 0x0F, 0xFF, 0x00, 0x0F, 0x00, 0x78, 0x07, 0xFE, 0x07, 0xFC,
0x7F, 0x00, 0x38, 0xFF, 0x01, 0xFE, 0x01, 0xF8, 0x00, 0x06, 0x00, 0x30, 0x03, 0xFE, 0x01, 0xF0
};

#ifdef STANDALONE
const unsigned char temperature_icon [] = {
0x00, 0x0F, 0x00, 0x00, 0x00, 0x1F, 0x80, 0x00, 0x00, 0x39, 0xC0, 0x00, 0x00, 0x30, 0xCF, 0xC0,
0x00, 0x30, 0xCF, 0xC0, 0x00, 0x30, 0xC0, 0x00, 0x00, 0x30, 0xC0, 0x00, 0x00, 0x30, 0xCF, 0xC0,
0x00, 0x30, 0xCF, 0xC0, 0x00, 0x30, 0xC0, 0x00, 0x00, 0x30, 0xC0, 0x00, 0x00, 0x36, 0xCF, 0xC0,
0x00, 0x36, 0xCF, 0xC0, 0x00, 0x36, 0xC0, 0x00, 0x00, 0x36, 0xC0, 0x00, 0x00, 0x36, 0xC7, 0xC0,
0x00, 0x36, 0xC7, 0xC0, 0x00, 0x76, 0xE0, 0x00, 0x00, 0xF6, 0xF0, 0x00, 0x01, 0xCF, 0x38, 0x00,
0x01, 0xBF, 0xD8, 0x00, 0x03, 0xBF, 0xDC, 0x00, 0x03, 0x7F, 0xEC, 0x00, 0x03, 0x7F, 0xEC, 0x00,
0x03, 0x7F, 0xEC, 0x00, 0x03, 0x7F, 0xEC, 0x00, 0x03, 0xBF, 0xDC, 0x00, 0x01, 0x9F, 0xD8, 0x00,
0x01, 0xCF, 0x38, 0x00, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x7F, 0xE0, 0x00, 0x00, 0x1F, 0x80, 0x00
};

const unsigned char humidity_icon [] = {
0x00, 0x01, 0x80, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x07, 0xE0, 0x00,
0x00, 0x0E, 0x70, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x38, 0x1C, 0x00,
0x00, 0x70, 0x0E, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x01, 0xC0, 0x03, 0x80,
0x03, 0x9E, 0x01, 0xC0, 0x03, 0x3F, 0x00, 0xC0, 0x07, 0x33, 0x0C, 0xE0, 0x06, 0x33, 0x1C, 0x60,
0x06, 0x3F, 0x38, 0x60, 0x0C, 0x1E, 0x70, 0x30, 0x0C, 0x00, 0xE0, 0x30, 0x0C, 0x01, 0xC0, 0x30,
0x0C, 0x03, 0x80, 0x30, 0x0C, 0x07, 0x00, 0x30, 0x0C, 0x0E, 0x78, 0x30, 0x06, 0x1C, 0xFC, 0x60,
0x06, 0x38, 0xCC, 0x60, 0x07, 0x30, 0xCC, 0xE0, 0x03, 0x00, 0xFC, 0xC0, 0x01, 0x80, 0x79, 0x80,
0x00, 0xE0, 0x07, 0x00, 0x00, 0x78, 0x1E, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x07, 0xE0, 0x00
};

const unsigned char uv_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00,
0x00, 0x01, 0x80, 0x00, 0x06, 0x01, 0x80, 0x40, 0x03, 0x01, 0x80, 0xC0, 0x01, 0x80, 0x01, 0x80,
0x00, 0xC3, 0xC3, 0x00, 0x00, 0x4F, 0xF2, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x00, 0x3F, 0xFC, 0x00,
0x00, 0x3F, 0xFC, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x7F, 0x7F, 0xFE, 0xFC,
0x00, 0x7F, 0xFE, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x1F, 0xF8, 0x00,
0x00, 0x1F, 0xF8, 0x00, 0x00, 0x47, 0xE2, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x01, 0x80, 0x01, 0x80,
0x03, 0x00, 0x00, 0xC0, 0x02, 0x22, 0x44, 0x40, 0x00, 0x22, 0x4C, 0x00, 0x00, 0x22, 0x68, 0x00,
0x00, 0x22, 0x38, 0x00, 0x00, 0x32, 0x30, 0x00, 0x00, 0x1E, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lux_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00,
0x00, 0x10, 0x88, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x8E, 0x31, 0x00,
0x00, 0x10, 0x08, 0x00, 0x00, 0x20, 0x14, 0x00, 0x06, 0x40, 0x02, 0x20, 0x00, 0x48, 0x09, 0x40,
0x00, 0x84, 0x05, 0x00, 0x00, 0x82, 0x05, 0x00, 0x3C, 0x81, 0x81, 0x3C, 0x00, 0x81, 0x01, 0x00,
0x00, 0x80, 0x05, 0x00, 0x00, 0x80, 0x05, 0x00, 0x03, 0x40, 0x02, 0x40, 0x04, 0x60, 0x02, 0x20,
0x00, 0x30, 0x0C, 0x00, 0x00, 0x48, 0x18, 0x00, 0x00, 0x84, 0x21, 0x00, 0x00, 0x02, 0x40, 0x00,
0x00, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char hpa_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x80, 0x00, 0x00, 0x20, 0x40,
0x00, 0x00, 0x40, 0x20, 0x00, 0x00, 0x40, 0x3C, 0x04, 0x01, 0xC0, 0x06, 0x04, 0x03, 0x00, 0x03,
0x06, 0x46, 0x00, 0x01, 0x02, 0x44, 0x00, 0x01, 0x02, 0x64, 0x00, 0x01, 0x02, 0x24, 0x00, 0x03,
0x7E, 0x22, 0x00, 0x06, 0xC0, 0x21, 0xFF, 0xFC, 0x01, 0xF0, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x01, 0xF8, 0x1F, 0x80, 0x03, 0x0C, 0x18, 0x00,
0x06, 0x06, 0x08, 0x00, 0x1C, 0x02, 0x08, 0xFE, 0x60, 0x03, 0x09, 0x80, 0x40, 0x01, 0xCC, 0x80,
0x80, 0x00, 0x64, 0x80, 0x80, 0x00, 0x20, 0x80, 0x80, 0x00, 0x20, 0xC0, 0xC0, 0x00, 0x20, 0x40,
0x40, 0x00, 0x60, 0x40, 0x3F, 0xFF, 0xC0, 0x00, 0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char elevation_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x70, 0x00,
0x00, 0x10, 0xD0, 0x00, 0x00, 0x19, 0x98, 0xC0, 0x00, 0x2D, 0x01, 0xE0, 0x00, 0x67, 0x03, 0xF0,
0x00, 0x42, 0x06, 0x58, 0x00, 0x80, 0x00, 0x40, 0x00, 0x80, 0x00, 0x40, 0x01, 0x8C, 0x70, 0x40,
0x03, 0x7B, 0x9C, 0x40, 0x02, 0x00, 0x00, 0x40, 0x06, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x40,
0x08, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x08, 0x30, 0x00, 0x00, 0x0C,
0x20, 0x00, 0x00, 0x04, 0x7F, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char aqi_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00,
0x00, 0xC0, 0x03, 0x10, 0x00, 0xDE, 0x03, 0x70, 0x01, 0xDF, 0xC3, 0xE0, 0x07, 0xC1, 0xFF, 0x80,
0x0E, 0xC0, 0x7E, 0x00, 0x08, 0xC0, 0x00, 0x10, 0x00, 0xDE, 0x00, 0x70, 0x01, 0xDF, 0x81, 0xE0,
0x07, 0xC1, 0xFF, 0x80, 0x0E, 0xC0, 0x7E, 0x00, 0x08, 0xC0, 0x00, 0x10, 0x00, 0xDE, 0x00, 0x70,
0x01, 0xDF, 0x81, 0xE0, 0x07, 0xC1, 0xFF, 0x80, 0x0E, 0xC0, 0x7F, 0x00, 0x08, 0xC0, 0x03, 0x00,
0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char co2_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x01, 0x80, 0xC0,
0x00, 0x03, 0x00, 0x60, 0x00, 0x02, 0x00, 0x20, 0x00, 0x06, 0x00, 0x30, 0x00, 0x3C, 0x00, 0x10,
0x00, 0x60, 0x00, 0x10, 0x00, 0x80, 0x00, 0x10, 0x01, 0x80, 0x00, 0x10, 0x1F, 0x00, 0x00, 0x18,
0x30, 0x1C, 0xE0, 0x0C, 0x40, 0x31, 0x27, 0x02, 0xC0, 0x21, 0x11, 0x03, 0x80, 0x21, 0x32, 0x01,
0x80, 0x3D, 0xE6, 0x01, 0x80, 0x08, 0x47, 0x01, 0x80, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x02,
0x60, 0x00, 0x00, 0x06, 0x1F, 0xFF, 0xFF, 0xF8, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char motion_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x40, 0x62, 0x00, 0x00, 0xB0, 0x2D, 0x00, 0x00, 0xA3, 0xC5, 0x00, 0x00, 0xA2, 0xE5, 0x00,
0x00, 0xA1, 0xB5, 0x00, 0x00, 0xA1, 0x85, 0x00, 0x00, 0xB3, 0x8D, 0x00, 0x00, 0x4E, 0x42, 0x00,
0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char noise_icon [] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
0x00, 0x00, 0x03, 0x00, 0x00, 0x03, 0x8E, 0x00, 0x00, 0x07, 0x98, 0x00, 0x00, 0x1C, 0x80, 0x60,
0x00, 0x38, 0x87, 0xC0, 0x03, 0xE0, 0x8E, 0x00, 0x06, 0x00, 0x80, 0x00, 0x06, 0x00, 0x87, 0xF0,
0x06, 0x00, 0x87, 0xF0, 0x06, 0x00, 0x80, 0x00, 0x03, 0xE0, 0x8E, 0x00, 0x00, 0x38, 0x87, 0xC0,
0x00, 0x1C, 0x90, 0x60, 0x00, 0x07, 0x98, 0x00, 0x00, 0x03, 0x8E, 0x00, 0x00, 0x00, 0x03, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif // STANDALONE
