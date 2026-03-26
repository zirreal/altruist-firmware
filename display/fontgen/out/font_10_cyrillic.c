#include "font_10_cyrillic.h"

// U+0401 'Ё'
static const uint8_t bmp_10_cyrillic_0401[] = {
0xD0, 
0x00, 
0xF0, 
0x80, 
0x80, 
0xF0, 
0x80, 
0x80, 
0x80, 
0xF0, 
};

// U+0410 'А'
static const uint8_t bmp_10_cyrillic_0410[] = {
0x30, 
0x30, 
0x38, 
0x68, 
0x48, 
0x7C, 
0xC4, 
0x84, 
};

// U+0411 'Б'
static const uint8_t bmp_10_cyrillic_0411[] = {
0xF0, 
0x80, 
0x80, 
0xF0, 
0x88, 
0x88, 
0x88, 
0xF0, 
};

// U+0412 'В'
static const uint8_t bmp_10_cyrillic_0412[] = {
0xF0, 
0x88, 
0x88, 
0xF0, 
0x88, 
0x88, 
0x88, 
0xF0, 
};

// U+0413 'Г'
static const uint8_t bmp_10_cyrillic_0413[] = {
0xF0, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+0414 'Д'
static const uint8_t bmp_10_cyrillic_0414[] = {
0x3C, 
0x24, 
0x24, 
0x24, 
0x24, 
0x24, 
0x44, 
0xFE, 
0x82, 
0x82, 
};

// U+0415 'Е'
static const uint8_t bmp_10_cyrillic_0415[] = {
0xF0, 
0x80, 
0x80, 
0xF0, 
0x80, 
0x80, 
0x80, 
0xF0, 
};

// U+0416 'Ж'
static const uint8_t bmp_10_cyrillic_0416[] = {
0xC9, 0x80, 
0x49, 0x00, 
0x2A, 0x00, 
0x3E, 0x00, 
0x2A, 0x00, 
0x6B, 0x00, 
0x49, 0x00, 
0xC9, 0x80, 
};

// U+0417 'З'
static const uint8_t bmp_10_cyrillic_0417[] = {
0x70, 
0x88, 
0x08, 
0x30, 
0x18, 
0x08, 
0xC8, 
0x70, 
};

// U+0418 'И'
static const uint8_t bmp_10_cyrillic_0418[] = {
0x88, 
0x98, 
0x98, 
0xA8, 
0xA8, 
0xC8, 
0xC8, 
0x88, 
};

// U+0419 'Й'
static const uint8_t bmp_10_cyrillic_0419[] = {
0x50, 
0x70, 
0x88, 
0x98, 
0x98, 
0xA8, 
0xA8, 
0xC8, 
0xC8, 
0x88, 
};

// U+041A 'К'
static const uint8_t bmp_10_cyrillic_041A[] = {
0x98, 
0x90, 
0xA0, 
0xE0, 
0xA0, 
0x90, 
0x98, 
0x88, 
};

// U+041B 'Л'
static const uint8_t bmp_10_cyrillic_041B[] = {
0x7C, 
0x44, 
0x44, 
0x44, 
0x44, 
0x44, 
0xC4, 
0x84, 
};

// U+041C 'М'
static const uint8_t bmp_10_cyrillic_041C[] = {
0xC6, 
0xC6, 
0xC6, 
0xAA, 
0xAA, 
0xAA, 
0x92, 
0x92, 
};

// U+041D 'Н'
static const uint8_t bmp_10_cyrillic_041D[] = {
0x84, 
0x84, 
0x84, 
0xFC, 
0x84, 
0x84, 
0x84, 
0x84, 
};

// U+041E 'О'
static const uint8_t bmp_10_cyrillic_041E[] = {
0x78, 
0xC8, 
0x84, 
0x84, 
0x84, 
0x84, 
0xCC, 
0x78, 
};

// U+041F 'П'
static const uint8_t bmp_10_cyrillic_041F[] = {
0xF8, 
0x88, 
0x88, 
0x88, 
0x88, 
0x88, 
0x88, 
0x88, 
};

// U+0420 'Р'
static const uint8_t bmp_10_cyrillic_0420[] = {
0xF0, 
0x88, 
0x88, 
0x88, 
0xF0, 
0x80, 
0x80, 
0x80, 
};

// U+0421 'С'
static const uint8_t bmp_10_cyrillic_0421[] = {
0x70, 
0xC8, 
0x88, 
0x80, 
0x80, 
0x88, 
0xC8, 
0x70, 
};

// U+0422 'Т'
static const uint8_t bmp_10_cyrillic_0422[] = {
0xF8, 
0x20, 
0x20, 
0x20, 
0x20, 
0x20, 
0x20, 
0x20, 
};

// U+0423 'У'
static const uint8_t bmp_10_cyrillic_0423[] = {
0x8C, 
0x48, 
0x48, 
0x70, 
0x30, 
0x30, 
0x20, 
0x60, 
};

// U+0424 'Ф'
static const uint8_t bmp_10_cyrillic_0424[] = {
0x10, 
0x7C, 
0xD6, 
0x92, 
0x92, 
0xD6, 
0x7C, 
0x10, 
};

// U+0425 'Х'
static const uint8_t bmp_10_cyrillic_0425[] = {
0xCC, 
0x48, 
0x38, 
0x30, 
0x30, 
0x38, 
0x48, 
0xCC, 
};

// U+0426 'Ц'
static const uint8_t bmp_10_cyrillic_0426[] = {
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
0x84, 
0xFC, 
0x04, 
0x04, 
};

// U+0427 'Ч'
static const uint8_t bmp_10_cyrillic_0427[] = {
0x88, 
0x88, 
0x88, 
0x88, 
0x78, 
0x08, 
0x08, 
0x08, 
};

// U+0428 'Ш'
static const uint8_t bmp_10_cyrillic_0428[] = {
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0x88, 0x80, 
0xFF, 0x80, 
};

// U+0429 'Щ'
static const uint8_t bmp_10_cyrillic_0429[] = {
0x89, 0x00, 
0x89, 0x00, 
0x89, 0x00, 
0x89, 0x00, 
0x89, 0x00, 
0x89, 0x00, 
0x89, 0x00, 
0xFF, 0x80, 
0x00, 0x80, 
0x00, 0x80, 
};

// U+042A 'Ъ'
static const uint8_t bmp_10_cyrillic_042A[] = {
0xE0, 
0x20, 
0x20, 
0x3C, 
0x22, 
0x22, 
0x22, 
0x3C, 
};

// U+042B 'Ы'
static const uint8_t bmp_10_cyrillic_042B[] = {
0x82, 
0x82, 
0x82, 
0xF2, 
0x8A, 
0x8A, 
0x8A, 
0xF2, 
};

// U+042C 'Ь'
static const uint8_t bmp_10_cyrillic_042C[] = {
0x80, 
0x80, 
0x80, 
0xF0, 
0x88, 
0x88, 
0x88, 
0xF0, 
};

// U+042D 'Э'
static const uint8_t bmp_10_cyrillic_042D[] = {
0x70, 
0x98, 
0x08, 
0x78, 
0x08, 
0x88, 
0x98, 
0x70, 
};

// U+042E 'Ю'
static const uint8_t bmp_10_cyrillic_042E[] = {
0x9E, 
0xB2, 
0xA1, 
0xA1, 
0xE1, 
0xA1, 
0xB3, 
0x9E, 
};

// U+042F 'Я'
static const uint8_t bmp_10_cyrillic_042F[] = {
0x78, 
0x88, 
0x88, 
0x88, 
0x78, 
0x48, 
0xC8, 
0x88, 
};

// U+0430 'а'
static const uint8_t bmp_10_cyrillic_0430[] = {
0x60, 
0x90, 
0x70, 
0x90, 
0x90, 
0xF0, 
};

// U+0431 'б'
static const uint8_t bmp_10_cyrillic_0431[] = {
0x08, 
0x30, 
0x40, 
0xF0, 
0x88, 
0x88, 
0x88, 
0x88, 
0x70, 
};

// U+0432 'в'
static const uint8_t bmp_10_cyrillic_0432[] = {
0xF0, 
0x90, 
0x90, 
0xF0, 
0x90, 
0xF0, 
};

// U+0433 'г'
static const uint8_t bmp_10_cyrillic_0433[] = {
0xE0, 
0x80, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+0434 'д'
static const uint8_t bmp_10_cyrillic_0434[] = {
0x78, 
0x48, 
0x48, 
0x48, 
0x48, 
0xFC, 
0x84, 
0x84, 
};

// U+0435 'е'
static const uint8_t bmp_10_cyrillic_0435[] = {
0x70, 
0x90, 
0xF0, 
0x80, 
0x90, 
0x70, 
};

// U+0436 'ж'
static const uint8_t bmp_10_cyrillic_0436[] = {
0x92, 
0x54, 
0x54, 
0x7C, 
0x54, 
0x92, 
};

// U+0437 'з'
static const uint8_t bmp_10_cyrillic_0437[] = {
0x70, 
0x10, 
0x70, 
0x10, 
0x90, 
0x70, 
};

// U+0438 'и'
static const uint8_t bmp_10_cyrillic_0438[] = {
0x90, 
0xB0, 
0xB0, 
0xD0, 
0xD0, 
0x90, 
};

// U+0439 'й'
static const uint8_t bmp_10_cyrillic_0439[] = {
0x60, 
0x00, 
0x90, 
0xB0, 
0xB0, 
0xD0, 
0xD0, 
0x90, 
};

// U+043A 'к'
static const uint8_t bmp_10_cyrillic_043A[] = {
0x90, 
0xB0, 
0xA0, 
0xE0, 
0xA0, 
0x90, 
};

// U+043B 'л'
static const uint8_t bmp_10_cyrillic_043B[] = {
0x78, 
0x48, 
0x48, 
0x48, 
0x48, 
0x88, 
};

// U+043C 'м'
static const uint8_t bmp_10_cyrillic_043C[] = {
0xCC, 
0xCC, 
0xCC, 
0xB4, 
0xB4, 
0xB4, 
};

// U+043D 'н'
static const uint8_t bmp_10_cyrillic_043D[] = {
0x90, 
0x90, 
0x90, 
0xF0, 
0x90, 
0x90, 
};

// U+043E 'о'
static const uint8_t bmp_10_cyrillic_043E[] = {
0x70, 
0x88, 
0x88, 
0x88, 
0x88, 
0x70, 
};

// U+043F 'п'
static const uint8_t bmp_10_cyrillic_043F[] = {
0xF0, 
0x90, 
0x90, 
0x90, 
0x90, 
0x90, 
};

// U+0440 'р'
static const uint8_t bmp_10_cyrillic_0440[] = {
0xF0, 
0x88, 
0x88, 
0x88, 
0x88, 
0xF0, 
0x80, 
0x80, 
};

// U+0441 'с'
static const uint8_t bmp_10_cyrillic_0441[] = {
0x70, 
0x90, 
0x80, 
0x80, 
0x90, 
0x70, 
};

// U+0442 'т'
static const uint8_t bmp_10_cyrillic_0442[] = {
0xF8, 
0x20, 
0x20, 
0x20, 
0x20, 
0x20, 
};

// U+0443 'у'
static const uint8_t bmp_10_cyrillic_0443[] = {
0x90, 
0x90, 
0xF0, 
0x60, 
0x60, 
0x60, 
0x40, 
0xC0, 
};

// U+0444 'ф'
static const uint8_t bmp_10_cyrillic_0444[] = {
0x10, 
0x10, 
0x78, 
0x94, 
0x94, 
0x94, 
0x94, 
0x78, 
0x10, 
0x10, 
};

// U+0445 'х'
static const uint8_t bmp_10_cyrillic_0445[] = {
0xD8, 
0x50, 
0x20, 
0x70, 
0x50, 
0xD8, 
};

// U+0446 'ц'
static const uint8_t bmp_10_cyrillic_0446[] = {
0x90, 
0x90, 
0x90, 
0x90, 
0x90, 
0xF8, 
0x08, 
0x08, 
};

// U+0447 'ч'
static const uint8_t bmp_10_cyrillic_0447[] = {
0x90, 
0x90, 
0x90, 
0x70, 
0x10, 
0x10, 
};

// U+0448 'ш'
static const uint8_t bmp_10_cyrillic_0448[] = {
0x92, 
0x92, 
0x92, 
0x92, 
0x92, 
0xFE, 
};

// U+0449 'щ'
static const uint8_t bmp_10_cyrillic_0449[] = {
0x92, 
0x92, 
0x92, 
0x92, 
0x92, 
0xFF, 
0x01, 
0x01, 
};

// U+044A 'ъ'
static const uint8_t bmp_10_cyrillic_044A[] = {
0xE0, 
0x20, 
0x3C, 
0x24, 
0x24, 
0x3C, 
};

// U+044B 'ы'
static const uint8_t bmp_10_cyrillic_044B[] = {
0x84, 
0x84, 
0xF4, 
0x94, 
0x94, 
0xF4, 
};

// U+044C 'ь'
static const uint8_t bmp_10_cyrillic_044C[] = {
0x80, 
0x80, 
0xF0, 
0x90, 
0x90, 
0xF0, 
};

// U+044D 'э'
static const uint8_t bmp_10_cyrillic_044D[] = {
0x70, 
0x48, 
0x38, 
0x08, 
0x48, 
0x70, 
};

// U+044E 'ю'
static const uint8_t bmp_10_cyrillic_044E[] = {
0x9C, 
0xB2, 
0xA2, 
0xE2, 
0xB2, 
0x9C, 
};

// U+044F 'я'
static const uint8_t bmp_10_cyrillic_044F[] = {
0x70, 
0x90, 
0x90, 
0x70, 
0x50, 
0x90, 
};

// U+0451 'ё'
static const uint8_t bmp_10_cyrillic_0451[] = {
0x50, 
0x00, 
0x70, 
0x90, 
0xF0, 
0x80, 
0x90, 
0x70, 
};

// U+00B0 '°'
static const uint8_t bmp_10_cyrillic_00B0[] = {
0xE0, 
0xA0, 
0xE0, 
};

// U+00B5 'µ'
static const uint8_t bmp_10_cyrillic_00B5[] = {
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
static const uint8_t bmp_10_cyrillic_00B2[] = {
0xE0, 
0x20, 
0x40, 
0xE0, 
};

// U+00B3 '³'
static const uint8_t bmp_10_cyrillic_00B3[] = {
0xE0, 
0x60, 
0x20, 
0xE0, 
};

// U+2082 '₂'
static const uint8_t bmp_10_cyrillic_2082[] = {
0xE0, 
0x20, 
0x40, 
0xE0, 
};

static const Glyph glyphs_10_cyrillic[] = {
  { 0x0401, 4, 10, 0, bmp_10_cyrillic_0401 }, // 'Ё'
  { 0x0410, 6, 8, 1, bmp_10_cyrillic_0410 }, // 'А'
  { 0x0411, 5, 8, 1, bmp_10_cyrillic_0411 }, // 'Б'
  { 0x0412, 5, 8, 1, bmp_10_cyrillic_0412 }, // 'В'
  { 0x0413, 4, 8, 1, bmp_10_cyrillic_0413 }, // 'Г'
  { 0x0414, 7, 10, 1, bmp_10_cyrillic_0414 }, // 'Д'
  { 0x0415, 4, 8, 1, bmp_10_cyrillic_0415 }, // 'Е'
  { 0x0416, 9, 8, 1, bmp_10_cyrillic_0416 }, // 'Ж'
  { 0x0417, 5, 8, 1, bmp_10_cyrillic_0417 }, // 'З'
  { 0x0418, 5, 8, 1, bmp_10_cyrillic_0418 }, // 'И'
  { 0x0419, 5, 10, 0, bmp_10_cyrillic_0419 }, // 'Й'
  { 0x041A, 6, 8, 1, bmp_10_cyrillic_041A }, // 'К'
  { 0x041B, 6, 8, 1, bmp_10_cyrillic_041B }, // 'Л'
  { 0x041C, 7, 8, 1, bmp_10_cyrillic_041C }, // 'М'
  { 0x041D, 6, 8, 1, bmp_10_cyrillic_041D }, // 'Н'
  { 0x041E, 6, 8, 1, bmp_10_cyrillic_041E }, // 'О'
  { 0x041F, 5, 8, 1, bmp_10_cyrillic_041F }, // 'П'
  { 0x0420, 5, 8, 1, bmp_10_cyrillic_0420 }, // 'Р'
  { 0x0421, 5, 8, 1, bmp_10_cyrillic_0421 }, // 'С'
  { 0x0422, 5, 8, 1, bmp_10_cyrillic_0422 }, // 'Т'
  { 0x0423, 6, 8, 1, bmp_10_cyrillic_0423 }, // 'У'
  { 0x0424, 7, 8, 1, bmp_10_cyrillic_0424 }, // 'Ф'
  { 0x0425, 6, 8, 1, bmp_10_cyrillic_0425 }, // 'Х'
  { 0x0426, 6, 10, 1, bmp_10_cyrillic_0426 }, // 'Ц'
  { 0x0427, 5, 8, 1, bmp_10_cyrillic_0427 }, // 'Ч'
  { 0x0428, 9, 8, 1, bmp_10_cyrillic_0428 }, // 'Ш'
  { 0x0429, 9, 10, 1, bmp_10_cyrillic_0429 }, // 'Щ'
  { 0x042A, 7, 8, 1, bmp_10_cyrillic_042A }, // 'Ъ'
  { 0x042B, 7, 8, 1, bmp_10_cyrillic_042B }, // 'Ы'
  { 0x042C, 5, 8, 1, bmp_10_cyrillic_042C }, // 'Ь'
  { 0x042D, 5, 8, 1, bmp_10_cyrillic_042D }, // 'Э'
  { 0x042E, 8, 8, 1, bmp_10_cyrillic_042E }, // 'Ю'
  { 0x042F, 5, 8, 1, bmp_10_cyrillic_042F }, // 'Я'
  { 0x0430, 4, 6, 3, bmp_10_cyrillic_0430 }, // 'а'
  { 0x0431, 5, 9, 0, bmp_10_cyrillic_0431 }, // 'б'
  { 0x0432, 5, 6, 3, bmp_10_cyrillic_0432 }, // 'в'
  { 0x0433, 3, 6, 3, bmp_10_cyrillic_0433 }, // 'г'
  { 0x0434, 6, 8, 3, bmp_10_cyrillic_0434 }, // 'д'
  { 0x0435, 5, 6, 3, bmp_10_cyrillic_0435 }, // 'е'
  { 0x0436, 7, 6, 3, bmp_10_cyrillic_0436 }, // 'ж'
  { 0x0437, 5, 6, 3, bmp_10_cyrillic_0437 }, // 'з'
  { 0x0438, 4, 6, 3, bmp_10_cyrillic_0438 }, // 'и'
  { 0x0439, 4, 8, 1, bmp_10_cyrillic_0439 }, // 'й'
  { 0x043A, 5, 6, 3, bmp_10_cyrillic_043A }, // 'к'
  { 0x043B, 5, 6, 3, bmp_10_cyrillic_043B }, // 'л'
  { 0x043C, 6, 6, 3, bmp_10_cyrillic_043C }, // 'м'
  { 0x043D, 4, 6, 3, bmp_10_cyrillic_043D }, // 'н'
  { 0x043E, 5, 6, 3, bmp_10_cyrillic_043E }, // 'о'
  { 0x043F, 4, 6, 3, bmp_10_cyrillic_043F }, // 'п'
  { 0x0440, 5, 8, 3, bmp_10_cyrillic_0440 }, // 'р'
  { 0x0441, 5, 6, 3, bmp_10_cyrillic_0441 }, // 'с'
  { 0x0442, 5, 6, 3, bmp_10_cyrillic_0442 }, // 'т'
  { 0x0443, 4, 8, 3, bmp_10_cyrillic_0443 }, // 'у'
  { 0x0444, 6, 10, 1, bmp_10_cyrillic_0444 }, // 'ф'
  { 0x0445, 5, 6, 3, bmp_10_cyrillic_0445 }, // 'х'
  { 0x0446, 5, 8, 3, bmp_10_cyrillic_0446 }, // 'ц'
  { 0x0447, 4, 6, 3, bmp_10_cyrillic_0447 }, // 'ч'
  { 0x0448, 7, 6, 3, bmp_10_cyrillic_0448 }, // 'ш'
  { 0x0449, 8, 8, 3, bmp_10_cyrillic_0449 }, // 'щ'
  { 0x044A, 7, 6, 3, bmp_10_cyrillic_044A }, // 'ъ'
  { 0x044B, 6, 6, 3, bmp_10_cyrillic_044B }, // 'ы'
  { 0x044C, 5, 6, 3, bmp_10_cyrillic_044C }, // 'ь'
  { 0x044D, 5, 6, 3, bmp_10_cyrillic_044D }, // 'э'
  { 0x044E, 7, 6, 3, bmp_10_cyrillic_044E }, // 'ю'
  { 0x044F, 4, 6, 3, bmp_10_cyrillic_044F }, // 'я'
  { 0x0451, 5, 8, 1, bmp_10_cyrillic_0451 }, // 'ё'
  { 0x00B0, 3, 3, 1, bmp_10_cyrillic_00B0 }, // '°'
  { 0x00B5, 4, 8, 3, bmp_10_cyrillic_00B5 }, // 'µ'
  { 0x00B2, 3, 4, 2, bmp_10_cyrillic_00B2 }, // '²'
  { 0x00B3, 3, 4, 2, bmp_10_cyrillic_00B3 }, // '³'
  { 0x2082, 3, 4, 7, bmp_10_cyrillic_2082 }, // '₂'
};

const Font font_10_cyrillic = {
  71, 10, glyphs_10_cyrillic
};
