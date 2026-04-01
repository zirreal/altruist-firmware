#ifndef FONT_10_ASCII_H
#define FONT_10_ASCII_H

#include <stdint.h>

typedef struct {
  uint32_t codepoint;
  uint16_t width;
  uint16_t height;
  uint16_t top;
  const uint8_t* bitmap;
} Glyph;

typedef struct {
  uint16_t count;
  uint16_t line_height;
  const Glyph* glyphs;
} Font;

extern const Font font_10_ascii;

#endif
