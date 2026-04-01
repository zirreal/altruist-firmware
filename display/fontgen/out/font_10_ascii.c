#include "font_10_ascii.h"

// U+0020 ' '
static const uint8_t bmp_10_ascii_0020[] = {
0x00, 
};

// U+0021 '!'
static const uint8_t bmp_10_ascii_0021[] = {
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x00, 
0x80, 
};

// U+0022 '"'
static const uint8_t bmp_10_ascii_0022[] = {
0xA0, 
};

// U+0023 '#'
static const uint8_t bmp_10_ascii_0023[] = {
0x22, 
0xFE, 
0x64, 
0x44, 
0xFE, 
0xC8, 
0x88, 
};

// U+0024 '$'
static const uint8_t bmp_10_ascii_0024[] = {
0x10, 
0xFE, 
0x92, 
0x90, 
0xFE, 
0x12, 
0x92, 
0xFE, 
0x10, 
};

// U+0025 '%'
static const uint8_t bmp_10_ascii_0025[] = {
0xF1, 0x00, 
0x93, 0x00, 
0xF6, 0x00, 
0x0F, 0x80, 
0x34, 0x80, 
0x64, 0x80, 
0x47, 0x80, 
};

// U+0026 '&'
static const uint8_t bmp_10_ascii_0026[] = {
0xFC, 
0x82, 
0x80, 
0xE0, 
0x9A, 
0x86, 
0xFF, 
};

// U+0027 '''
static const uint8_t bmp_10_ascii_0027[] = {
0x80, 
};

// U+0028 '('
static const uint8_t bmp_10_ascii_0028[] = {
0xC0, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0xC0, 
};

// U+0029 ')'
static const uint8_t bmp_10_ascii_0029[] = {
0xC0, 
0x40, 
0x40, 
0x40, 
0x40, 
0x40, 
0xC0, 
};

// U+002A '*'
static const uint8_t bmp_10_ascii_002A[] = {
0x20, 
0xF8, 
0x20, 
0x50, 
};

// U+002B '+'
static const uint8_t bmp_10_ascii_002B[] = {
0x40, 
0xF0, 
0x40, 
0x40, 
};

// U+002C ','
static const uint8_t bmp_10_ascii_002C[] = {
0x80, 
0x80, 
};

// U+002D '-'
static const uint8_t bmp_10_ascii_002D[] = {
0xF0, 
};

// U+002E '.'
static const uint8_t bmp_10_ascii_002E[] = {
0x80, 
};

// U+002F '/'
static const uint8_t bmp_10_ascii_002F[] = {
0x08, 
0x10, 
0x30, 
0x20, 
0x40, 
0x80, 
0x80, 
};

// U+0030 '0'
static const uint8_t bmp_10_ascii_0030[] = {
0xFE, 
0x86, 
0x8E, 
0x92, 
0xE2, 
0xC2, 
0xFE, 
};

// U+0031 '1'
static const uint8_t bmp_10_ascii_0031[] = {
0x60, 
0xE0, 
0x20, 
0x20, 
0x20, 
0x20, 
0x20, 
};

// U+0032 '2'
static const uint8_t bmp_10_ascii_0032[] = {
0xFE, 
0x82, 
0x02, 
0xFE, 
0x80, 
0x80, 
0xFE, 
};

// U+0033 '3'
static const uint8_t bmp_10_ascii_0033[] = {
0xFE, 
0x82, 
0x02, 
0x7E, 
0x02, 
0x82, 
0xFE, 
};

// U+0034 '4'
static const uint8_t bmp_10_ascii_0034[] = {
0x0C, 
0x1C, 
0x34, 
0x44, 
0xFE, 
0x04, 
0x04, 
};

// U+0035 '5'
static const uint8_t bmp_10_ascii_0035[] = {
0xFE, 
0x80, 
0x80, 
0xFE, 
0x02, 
0x82, 
0xFE, 
};

// U+0036 '6'
static const uint8_t bmp_10_ascii_0036[] = {
0xFC, 
0x80, 
0x80, 
0xFE, 
0x82, 
0x82, 
0xFE, 
};

// U+0037 '7'
static const uint8_t bmp_10_ascii_0037[] = {
0xFC, 
0x04, 
0x04, 
0x04, 
0x04, 
0x04, 
0x04, 
};

// U+0038 '8'
static const uint8_t bmp_10_ascii_0038[] = {
0xFE, 
0x82, 
0x82, 
0xFE, 
0x82, 
0x82, 
0xFE, 
};

// U+0039 '9'
static const uint8_t bmp_10_ascii_0039[] = {
0xFE, 
0x82, 
0x82, 
0xFE, 
0x02, 
0x02, 
0xFE, 
};

// U+003A ':'
static const uint8_t bmp_10_ascii_003A[] = {
0x80, 
0x00, 
0x00, 
0x00, 
0x00, 
0x80, 
};

// U+003B ';'
static const uint8_t bmp_10_ascii_003B[] = {
0x80, 
0x00, 
0x00, 
0x00, 
0x00, 
0x80, 
0x80, 
};

// U+003C '<'
static const uint8_t bmp_10_ascii_003C[] = {
0x10, 
0x70, 
0xC0, 
0xC0, 
0x30, 
0x10, 
};

// U+003D '='
static const uint8_t bmp_10_ascii_003D[] = {
0xF8, 
0x00, 
0xF8, 
};

// U+003E '>'
static const uint8_t bmp_10_ascii_003E[] = {
0x80, 
0xE0, 
0x30, 
0x30, 
0xC0, 
0x80, 
};

// U+003F '?'
static const uint8_t bmp_10_ascii_003F[] = {
0xFC, 
0x04, 
0x04, 
0x7C, 
0x40, 
0x00, 
0x40, 
};

// U+0040 '@'
static const uint8_t bmp_10_ascii_0040[] = {
0xFE, 
0x82, 
0xBA, 
0xAA, 
0xBE, 
0x80, 
0xFE, 
};

// U+0041 'A'
static const uint8_t bmp_10_ascii_0041[] = {
0xFE, 
0x82, 
0x82, 
0xFE, 
0x82, 
0x82, 
0x82, 
};

// U+0042 'B'
static const uint8_t bmp_10_ascii_0042[] = {
0xFE, 
0x82, 
0x82, 
0xFE, 
0x82, 
0x82, 
0xFE, 
};

// U+0043 'C'
static const uint8_t bmp_10_ascii_0043[] = {
0xFE, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0xFE, 
};

// U+0044 'D'
static const uint8_t bmp_10_ascii_0044[] = {
0xFE, 
0x82, 
0x82, 
0x82, 
0x82, 
0x82, 
0xFE, 
};

// U+0045 'E'
static const uint8_t bmp_10_ascii_0045[] = {
0xFE, 
0x80, 
0x80, 
0xF8, 
0x80, 
0x80, 
0xFE, 
};

// U+0046 'F'
static const uint8_t bmp_10_ascii_0046[] = {
0xFE, 
0x80, 
0x80, 
0xF8, 
0x80, 
0x80, 
0x80, 
};

// U+0047 'G'
static const uint8_t bmp_10_ascii_0047[] = {
0xFE, 
0x82, 
0x80, 
0x8E, 
0x82, 
0x82, 
0xFE, 
};

// U+0048 'H'
static const uint8_t bmp_10_ascii_0048[] = {
0x82, 
0x82, 
0x82, 
0xFE, 
0x82, 
0x82, 
0x82, 
};

// U+0049 'I'
static const uint8_t bmp_10_ascii_0049[] = {
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+004A 'J'
static const uint8_t bmp_10_ascii_004A[] = {
0x02, 
0x02, 
0x02, 
0x02, 
0x02, 
0x82, 
0xFE, 
};

// U+004B 'K'
static const uint8_t bmp_10_ascii_004B[] = {
0x86, 
0x8C, 
0x98, 
0xF0, 
0x98, 
0x8C, 
0x86, 
};

// U+004C 'L'
static const uint8_t bmp_10_ascii_004C[] = {
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0xFE, 
};

// U+004D 'M'
static const uint8_t bmp_10_ascii_004D[] = {
0xC3, 
0xE7, 
0xA5, 
0x99, 
0x81, 
0x81, 
0x81, 
};

// U+004E 'N'
static const uint8_t bmp_10_ascii_004E[] = {
0xC2, 
0xE2, 
0xA2, 
0x92, 
0x8A, 
0x8E, 
0x86, 
};

// U+004F 'O'
static const uint8_t bmp_10_ascii_004F[] = {
0xFE, 
0x82, 
0x82, 
0x82, 
0x82, 
0x82, 
0xFE, 
};

// U+0050 'P'
static const uint8_t bmp_10_ascii_0050[] = {
0xFE, 
0x82, 
0x82, 
0xFE, 
0x80, 
0x80, 
0x80, 
};

// U+0051 'Q'
static const uint8_t bmp_10_ascii_0051[] = {
0xFE, 
0x82, 
0x82, 
0x82, 
0x82, 
0x82, 
0xFF, 
};

// U+0052 'R'
static const uint8_t bmp_10_ascii_0052[] = {
0xFE, 
0x82, 
0x82, 
0xFE, 
0x88, 
0x84, 
0x86, 
};

// U+0053 'S'
static const uint8_t bmp_10_ascii_0053[] = {
0xFE, 
0x82, 
0x80, 
0xFE, 
0x02, 
0x82, 
0xFE, 
};

// U+0054 'T'
static const uint8_t bmp_10_ascii_0054[] = {
0xFE, 
0x10, 
0x10, 
0x10, 
0x10, 
0x10, 
0x10, 
};

// U+0055 'U'
static const uint8_t bmp_10_ascii_0055[] = {
0x82, 
0x82, 
0x82, 
0x82, 
0x82, 
0x82, 
0xFE, 
};

// U+0056 'V'
static const uint8_t bmp_10_ascii_0056[] = {
0x40, 0x80, 
0x60, 0x80, 
0x21, 0x00, 
0x33, 0x00, 
0x12, 0x00, 
0x0C, 0x00, 
0x0C, 0x00, 
};

// U+0057 'W'
static const uint8_t bmp_10_ascii_0057[] = {
0x46, 0x20, 
0x46, 0x20, 
0x4E, 0x60, 
0x29, 0x40, 
0x29, 0x40, 
0x31, 0x80, 
0x10, 0x80, 
};

// U+0058 'X'
static const uint8_t bmp_10_ascii_0058[] = {
0xC6, 
0x6C, 
0x28, 
0x10, 
0x28, 
0x6C, 
0xC6, 
};

// U+0059 'Y'
static const uint8_t bmp_10_ascii_0059[] = {
0x82, 
0x44, 
0x28, 
0x38, 
0x10, 
0x10, 
0x10, 
};

// U+005A 'Z'
static const uint8_t bmp_10_ascii_005A[] = {
0xFE, 
0x06, 
0x0C, 
0x10, 
0x60, 
0xC0, 
0xFE, 
};

// U+005B '['
static const uint8_t bmp_10_ascii_005B[] = {
0xC0, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0xC0, 
};

// U+005C '\'
static const uint8_t bmp_10_ascii_005C[] = {
0x80, 
0x80, 
0x40, 
0x20, 
0x10, 
0x08, 
0x08, 
};

// U+005D ']'
static const uint8_t bmp_10_ascii_005D[] = {
0xC0, 
0x40, 
0x40, 
0x40, 
0x40, 
0x40, 
0xC0, 
};

// U+005E '^'
static const uint8_t bmp_10_ascii_005E[] = {
0x20, 
0x60, 
0x60, 
0x90, 
};

// U+005F '_'
static const uint8_t bmp_10_ascii_005F[] = {
0xFE, 
};

// U+0060 '`'
static const uint8_t bmp_10_ascii_0060[] = {
0x80, 
};

// U+0061 'a'
static const uint8_t bmp_10_ascii_0061[] = {
0xFC, 
0x04, 
0xFC, 
0x84, 
0x84, 
0xFC, 
};

// U+0062 'b'
static const uint8_t bmp_10_ascii_0062[] = {
0x80, 
0x80, 
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
};

// U+0063 'c'
static const uint8_t bmp_10_ascii_0063[] = {
0xFC, 
0x80, 
0x80, 
0x80, 
0x80, 
0xFC, 
};

// U+0064 'd'
static const uint8_t bmp_10_ascii_0064[] = {
0x04, 
0x04, 
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
};

// U+0065 'e'
static const uint8_t bmp_10_ascii_0065[] = {
0xFC, 
0x84, 
0x84, 
0xFC, 
0x80, 
0xFC, 
};

// U+0066 'f'
static const uint8_t bmp_10_ascii_0066[] = {
0xE0, 
0x80, 
0xE0, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+0067 'g'
static const uint8_t bmp_10_ascii_0067[] = {
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
0x04, 
0x7C, 
};

// U+0068 'h'
static const uint8_t bmp_10_ascii_0068[] = {
0x80, 
0x80, 
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
};

// U+0069 'i'
static const uint8_t bmp_10_ascii_0069[] = {
0x80, 
0x00, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+006A 'j'
static const uint8_t bmp_10_ascii_006A[] = {
0x10, 
0x00, 
0x10, 
0x10, 
0x10, 
0x10, 
0x10, 
0x10, 
0x10, 
0xF0, 
};

// U+006B 'k'
static const uint8_t bmp_10_ascii_006B[] = {
0x80, 
0x80, 
0x8C, 
0x98, 
0xE0, 
0x90, 
0x88, 
0x8C, 
};

// U+006C 'l'
static const uint8_t bmp_10_ascii_006C[] = {
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0xC0, 
};

// U+006D 'm'
static const uint8_t bmp_10_ascii_006D[] = {
0xFF, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
};

// U+006E 'n'
static const uint8_t bmp_10_ascii_006E[] = {
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
};

// U+006F 'o'
static const uint8_t bmp_10_ascii_006F[] = {
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
};

// U+0070 'p'
static const uint8_t bmp_10_ascii_0070[] = {
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
0x80, 
0x80, 
};

// U+0071 'q'
static const uint8_t bmp_10_ascii_0071[] = {
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
0x04, 
0x04, 
};

// U+0072 'r'
static const uint8_t bmp_10_ascii_0072[] = {
0xF0, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+0073 's'
static const uint8_t bmp_10_ascii_0073[] = {
0xFC, 
0x80, 
0xFC, 
0x04, 
0x04, 
0xFC, 
};

// U+0074 't'
static const uint8_t bmp_10_ascii_0074[] = {
0x80, 
0x80, 
0xE0, 
0x80, 
0x80, 
0x80, 
0x80, 
0xE0, 
};

// U+0075 'u'
static const uint8_t bmp_10_ascii_0075[] = {
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
};

// U+0076 'v'
static const uint8_t bmp_10_ascii_0076[] = {
0xC2, 
0x42, 
0x24, 
0x24, 
0x18, 
0x18, 
};

// U+0077 'w'
static const uint8_t bmp_10_ascii_0077[] = {
0x44, 0x40, 
0x4E, 0x40, 
0x4A, 0x80, 
0x3B, 0x80, 
0x31, 0x80, 
0x31, 0x00, 
};

// U+0078 'x'
static const uint8_t bmp_10_ascii_0078[] = {
0xCC, 
0x78, 
0x30, 
0x30, 
0x48, 
0xCC, 
};

// U+0079 'y'
static const uint8_t bmp_10_ascii_0079[] = {
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
0x04, 
0x7C, 
};

// U+007A 'z'
static const uint8_t bmp_10_ascii_007A[] = {
0xFC, 
0x0C, 
0x10, 
0x20, 
0xC0, 
0xFC, 
};

// U+007B '{'
static const uint8_t bmp_10_ascii_007B[] = {
0x60, 
0x40, 
0x40, 
0x80, 
0x40, 
0x40, 
0x60, 
};

// U+007C '|'
static const uint8_t bmp_10_ascii_007C[] = {
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+007D '}'
static const uint8_t bmp_10_ascii_007D[] = {
0xC0, 
0x40, 
0x40, 
0x20, 
0x40, 
0x40, 
0xC0, 
};

// U+007E '~'
static const uint8_t bmp_10_ascii_007E[] = {
0xC0, 
0x30, 
};

// U+00B0 '°'
static const uint8_t bmp_10_ascii_00B0[] = {
0xF0, 
0x90, 
0x90, 
0xF0, 
};

// U+00B5 'µ'
static const uint8_t bmp_10_ascii_00B5[] = {
0x90, 
0x90, 
0x90, 
0x90, 
0x90, 
0xF0, 
0x80, 
0x80, 
};

// U+00B2 '²'
static const uint8_t bmp_10_ascii_00B2[] = {
0xE0, 
0x20, 
0x40, 
0xE0, 
};

// U+00B3 '³'
static const uint8_t bmp_10_ascii_00B3[] = {
0xE0, 
0x60, 
0x20, 
0xE0, 
};

// U+2082 '₂'
static const uint8_t bmp_10_ascii_2082[] = {
0xE0, 
0x20, 
0x40, 
0xE0, 
};

static const Glyph glyphs_10_ascii[] = {
  { 0x0020, 1, 1, 8, bmp_10_ascii_0020 }, // ' '
  { 0x0021, 1, 7, 2, bmp_10_ascii_0021 }, // '!'
  { 0x0022, 3, 1, 2, bmp_10_ascii_0022 }, // '"'
  { 0x0023, 7, 7, 2, bmp_10_ascii_0023 }, // '#'
  { 0x0024, 7, 9, 1, bmp_10_ascii_0024 }, // '$'
  { 0x0025, 9, 7, 2, bmp_10_ascii_0025 }, // '%'
  { 0x0026, 8, 7, 2, bmp_10_ascii_0026 }, // '&'
  { 0x0027, 1, 1, 2, bmp_10_ascii_0027 }, // '''
  { 0x0028, 2, 7, 2, bmp_10_ascii_0028 }, // '('
  { 0x0029, 2, 7, 2, bmp_10_ascii_0029 }, // ')'
  { 0x002A, 5, 4, 2, bmp_10_ascii_002A }, // '*'
  { 0x002B, 4, 4, 4, bmp_10_ascii_002B }, // '+'
  { 0x002C, 1, 2, 8, bmp_10_ascii_002C }, // ','
  { 0x002D, 4, 1, 5, bmp_10_ascii_002D }, // '-'
  { 0x002E, 1, 1, 8, bmp_10_ascii_002E }, // '.'
  { 0x002F, 5, 7, 2, bmp_10_ascii_002F }, // '/'
  { 0x0030, 7, 7, 2, bmp_10_ascii_0030 }, // '0'
  { 0x0031, 3, 7, 2, bmp_10_ascii_0031 }, // '1'
  { 0x0032, 7, 7, 2, bmp_10_ascii_0032 }, // '2'
  { 0x0033, 7, 7, 2, bmp_10_ascii_0033 }, // '3'
  { 0x0034, 7, 7, 2, bmp_10_ascii_0034 }, // '4'
  { 0x0035, 7, 7, 2, bmp_10_ascii_0035 }, // '5'
  { 0x0036, 7, 7, 2, bmp_10_ascii_0036 }, // '6'
  { 0x0037, 6, 7, 2, bmp_10_ascii_0037 }, // '7'
  { 0x0038, 7, 7, 2, bmp_10_ascii_0038 }, // '8'
  { 0x0039, 7, 7, 2, bmp_10_ascii_0039 }, // '9'
  { 0x003A, 1, 6, 3, bmp_10_ascii_003A }, // ':'
  { 0x003B, 1, 7, 3, bmp_10_ascii_003B }, // ';'
  { 0x003C, 4, 6, 3, bmp_10_ascii_003C }, // '<'
  { 0x003D, 5, 3, 4, bmp_10_ascii_003D }, // '='
  { 0x003E, 4, 6, 3, bmp_10_ascii_003E }, // '>'
  { 0x003F, 6, 7, 2, bmp_10_ascii_003F }, // '?'
  { 0x0040, 7, 7, 2, bmp_10_ascii_0040 }, // '@'
  { 0x0041, 7, 7, 2, bmp_10_ascii_0041 }, // 'A'
  { 0x0042, 7, 7, 2, bmp_10_ascii_0042 }, // 'B'
  { 0x0043, 7, 7, 2, bmp_10_ascii_0043 }, // 'C'
  { 0x0044, 7, 7, 2, bmp_10_ascii_0044 }, // 'D'
  { 0x0045, 7, 7, 2, bmp_10_ascii_0045 }, // 'E'
  { 0x0046, 7, 7, 2, bmp_10_ascii_0046 }, // 'F'
  { 0x0047, 7, 7, 2, bmp_10_ascii_0047 }, // 'G'
  { 0x0048, 7, 7, 2, bmp_10_ascii_0048 }, // 'H'
  { 0x0049, 1, 7, 2, bmp_10_ascii_0049 }, // 'I'
  { 0x004A, 7, 7, 2, bmp_10_ascii_004A }, // 'J'
  { 0x004B, 7, 7, 2, bmp_10_ascii_004B }, // 'K'
  { 0x004C, 7, 7, 2, bmp_10_ascii_004C }, // 'L'
  { 0x004D, 8, 7, 2, bmp_10_ascii_004D }, // 'M'
  { 0x004E, 7, 7, 2, bmp_10_ascii_004E }, // 'N'
  { 0x004F, 7, 7, 2, bmp_10_ascii_004F }, // 'O'
  { 0x0050, 7, 7, 2, bmp_10_ascii_0050 }, // 'P'
  { 0x0051, 8, 7, 2, bmp_10_ascii_0051 }, // 'Q'
  { 0x0052, 7, 7, 2, bmp_10_ascii_0052 }, // 'R'
  { 0x0053, 7, 7, 2, bmp_10_ascii_0053 }, // 'S'
  { 0x0054, 7, 7, 2, bmp_10_ascii_0054 }, // 'T'
  { 0x0055, 7, 7, 2, bmp_10_ascii_0055 }, // 'U'
  { 0x0056, 10, 7, 2, bmp_10_ascii_0056 }, // 'V'
  { 0x0057, 11, 7, 2, bmp_10_ascii_0057 }, // 'W'
  { 0x0058, 7, 7, 2, bmp_10_ascii_0058 }, // 'X'
  { 0x0059, 7, 7, 2, bmp_10_ascii_0059 }, // 'Y'
  { 0x005A, 7, 7, 2, bmp_10_ascii_005A }, // 'Z'
  { 0x005B, 2, 7, 2, bmp_10_ascii_005B }, // '['
  { 0x005C, 5, 7, 2, bmp_10_ascii_005C }, // '\'
  { 0x005D, 2, 7, 2, bmp_10_ascii_005D }, // ']'
  { 0x005E, 4, 4, 1, bmp_10_ascii_005E }, // '^'
  { 0x005F, 7, 1, 9, bmp_10_ascii_005F }, // '_'
  { 0x0060, 1, 1, 1, bmp_10_ascii_0060 }, // '`'
  { 0x0061, 6, 6, 3, bmp_10_ascii_0061 }, // 'a'
  { 0x0062, 6, 8, 1, bmp_10_ascii_0062 }, // 'b'
  { 0x0063, 6, 6, 3, bmp_10_ascii_0063 }, // 'c'
  { 0x0064, 6, 8, 1, bmp_10_ascii_0064 }, // 'd'
  { 0x0065, 6, 6, 3, bmp_10_ascii_0065 }, // 'e'
  { 0x0066, 3, 8, 1, bmp_10_ascii_0066 }, // 'f'
  { 0x0067, 6, 8, 3, bmp_10_ascii_0067 }, // 'g'
  { 0x0068, 6, 8, 1, bmp_10_ascii_0068 }, // 'h'
  { 0x0069, 1, 8, 1, bmp_10_ascii_0069 }, // 'i'
  { 0x006A, 4, 10, 1, bmp_10_ascii_006A }, // 'j'
  { 0x006B, 6, 8, 1, bmp_10_ascii_006B }, // 'k'
  { 0x006C, 2, 8, 1, bmp_10_ascii_006C }, // 'l'
  { 0x006D, 9, 6, 3, bmp_10_ascii_006D }, // 'm'
  { 0x006E, 6, 6, 3, bmp_10_ascii_006E }, // 'n'
  { 0x006F, 6, 6, 3, bmp_10_ascii_006F }, // 'o'
  { 0x0070, 6, 8, 3, bmp_10_ascii_0070 }, // 'p'
  { 0x0071, 6, 8, 3, bmp_10_ascii_0071 }, // 'q'
  { 0x0072, 4, 6, 3, bmp_10_ascii_0072 }, // 'r'
  { 0x0073, 6, 6, 3, bmp_10_ascii_0073 }, // 's'
  { 0x0074, 3, 8, 1, bmp_10_ascii_0074 }, // 't'
  { 0x0075, 6, 6, 3, bmp_10_ascii_0075 }, // 'u'
  { 0x0076, 8, 6, 3, bmp_10_ascii_0076 }, // 'v'
  { 0x0077, 10, 6, 3, bmp_10_ascii_0077 }, // 'w'
  { 0x0078, 6, 6, 3, bmp_10_ascii_0078 }, // 'x'
  { 0x0079, 6, 8, 3, bmp_10_ascii_0079 }, // 'y'
  { 0x007A, 6, 6, 3, bmp_10_ascii_007A }, // 'z'
  { 0x007B, 3, 7, 2, bmp_10_ascii_007B }, // '{'
  { 0x007C, 1, 9, 1, bmp_10_ascii_007C }, // '|'
  { 0x007D, 3, 7, 2, bmp_10_ascii_007D }, // '}'
  { 0x007E, 4, 2, 5, bmp_10_ascii_007E }, // '~'
  { 0x00B0, 4, 4, 1, bmp_10_ascii_00B0 }, // '°'
  { 0x00B5, 4, 8, 3, bmp_10_ascii_00B5 }, // 'µ'
  { 0x00B2, 3, 4, 2, bmp_10_ascii_00B2 }, // '²'
  { 0x00B3, 3, 4, 2, bmp_10_ascii_00B3 }, // '³'
  { 0x2082, 3, 4, 7, bmp_10_ascii_2082 }, // '₂'
};

const Font font_10_ascii = {
  100, 10, glyphs_10_ascii
};
