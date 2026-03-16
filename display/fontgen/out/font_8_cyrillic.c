#include "font_8_cyrillic.h"

// U+0401 'Ё'
static const uint8_t bmp_8_cyrillic_0401[] = {
0xA0, 
0x00, 
0xF0, 
0x80, 
0xE0, 
0x80, 
0xF0, 
};

// U+0410 'А'
static const uint8_t bmp_8_cyrillic_0410[] = {
0x20, 
0x50, 
0x50, 
0x78, 
0x88, 
};

// U+0411 'Б'
static const uint8_t bmp_8_cyrillic_0411[] = {
0xE0, 
0x80, 
0xF0, 
0x90, 
0xF0, 
};

// U+0412 'В'
static const uint8_t bmp_8_cyrillic_0412[] = {
0xF0, 
0x90, 
0xE0, 
0x90, 
0xF0, 
};

// U+0413 'Г'
static const uint8_t bmp_8_cyrillic_0413[] = {
0xF0, 
0x80, 
0x80, 
0x80, 
0x80, 
};

// U+0414 'Д'
static const uint8_t bmp_8_cyrillic_0414[] = {
0x78, 
0x48, 
0x48, 
0x48, 
0xFC, 
0x84, 
0x84, 
};

// U+0415 'Е'
static const uint8_t bmp_8_cyrillic_0415[] = {
0xF0, 
0x80, 
0xE0, 
0x80, 
0xF0, 
};

// U+0416 'Ж'
static const uint8_t bmp_8_cyrillic_0416[] = {
0x92, 
0x54, 
0x7C, 
0x54, 
0x92, 
};

// U+0417 'З'
static const uint8_t bmp_8_cyrillic_0417[] = {
0xF0, 
0x10, 
0x70, 
0x90, 
0xE0, 
};

// U+0418 'И'
static const uint8_t bmp_8_cyrillic_0418[] = {
0x90, 
0xB0, 
0xD0, 
0xD0, 
0x90, 
};

// U+0419 'Й'
static const uint8_t bmp_8_cyrillic_0419[] = {
0x60, 
0x60, 
0x90, 
0xB0, 
0xD0, 
0xD0, 
0x90, 
};

// U+041A 'К'
static const uint8_t bmp_8_cyrillic_041A[] = {
0x90, 
0xA0, 
0xE0, 
0xA0, 
0x90, 
};

// U+041B 'Л'
static const uint8_t bmp_8_cyrillic_041B[] = {
0x78, 
0x48, 
0x48, 
0x48, 
0x88, 
};

// U+041C 'М'
static const uint8_t bmp_8_cyrillic_041C[] = {
0xCC, 
0xCC, 
0xD4, 
0xB4, 
0xB4, 
};

// U+041D 'Н'
static const uint8_t bmp_8_cyrillic_041D[] = {
0x88, 
0x88, 
0xF8, 
0x88, 
0x88, 
};

// U+041E 'О'
static const uint8_t bmp_8_cyrillic_041E[] = {
0x70, 
0x88, 
0x88, 
0x88, 
0x70, 
};

// U+041F 'П'
static const uint8_t bmp_8_cyrillic_041F[] = {
0xF8, 
0x88, 
0x88, 
0x88, 
0x88, 
};

// U+0420 'Р'
static const uint8_t bmp_8_cyrillic_0420[] = {
0xF0, 
0x90, 
0xF0, 
0x80, 
0x80, 
};

// U+0421 'С'
static const uint8_t bmp_8_cyrillic_0421[] = {
0x70, 
0x90, 
0x80, 
0x90, 
0x70, 
};

// U+0422 'Т'
static const uint8_t bmp_8_cyrillic_0422[] = {
0xF8, 
0x20, 
0x20, 
0x20, 
0x20, 
};

// U+0423 'У'
static const uint8_t bmp_8_cyrillic_0423[] = {
0xC8, 
0x50, 
0x30, 
0x20, 
0x60, 
};

// U+0424 'Ф'
static const uint8_t bmp_8_cyrillic_0424[] = {
0x20, 
0x70, 
0xA8, 
0xA8, 
0x70, 
0x20, 
};

// U+0425 'Х'
static const uint8_t bmp_8_cyrillic_0425[] = {
0x58, 
0x50, 
0x20, 
0x50, 
0xD8, 
};

// U+0426 'Ц'
static const uint8_t bmp_8_cyrillic_0426[] = {
0x90, 
0x90, 
0x90, 
0x90, 
0xF8, 
0x08, 
0x08, 
};

// U+0427 'Ч'
static const uint8_t bmp_8_cyrillic_0427[] = {
0x90, 
0x90, 
0xF0, 
0x10, 
0x10, 
};

// U+0428 'Ш'
static const uint8_t bmp_8_cyrillic_0428[] = {
0x92, 
0x92, 
0x92, 
0x92, 
0xFE, 
};

// U+0429 'Щ'
static const uint8_t bmp_8_cyrillic_0429[] = {
0x94, 
0x94, 
0x94, 
0x94, 
0xFE, 
0x02, 
0x02, 
};

// U+042A 'Ъ'
static const uint8_t bmp_8_cyrillic_042A[] = {
0xE0, 
0x20, 
0x3C, 
0x24, 
0x3C, 
};

// U+042B 'Ы'
static const uint8_t bmp_8_cyrillic_042B[] = {
0x84, 
0x84, 
0xF4, 
0x94, 
0xF4, 
};

// U+042C 'Ь'
static const uint8_t bmp_8_cyrillic_042C[] = {
0x80, 
0x80, 
0xF0, 
0x90, 
0xF0, 
};

// U+042D 'Э'
static const uint8_t bmp_8_cyrillic_042D[] = {
0x70, 
0xC8, 
0x38, 
0xC8, 
0x70, 
};

// U+042E 'Ю'
static const uint8_t bmp_8_cyrillic_042E[] = {
0x98, 
0xA4, 
0xE4, 
0xA4, 
0x98, 
};

// U+042F 'Я'
static const uint8_t bmp_8_cyrillic_042F[] = {
0xF0, 
0x90, 
0x70, 
0xD0, 
0x90, 
};

// U+0430 'а'
static const uint8_t bmp_8_cyrillic_0430[] = {
0x70, 
0x70, 
0x90, 
0x70, 
};

// U+0431 'б'
static const uint8_t bmp_8_cyrillic_0431[] = {
0x10, 
0x60, 
0xE0, 
0x90, 
0x90, 
0x60, 
};

// U+0432 'в'
static const uint8_t bmp_8_cyrillic_0432[] = {
0xE0, 
0x90, 
0xF0, 
0xF0, 
};

// U+0433 'г'
static const uint8_t bmp_8_cyrillic_0433[] = {
0xE0, 
0x80, 
0x80, 
0x80, 
};

// U+0434 'д'
static const uint8_t bmp_8_cyrillic_0434[] = {
0x70, 
0x50, 
0x50, 
0xF8, 
0x88, 
};

// U+0435 'е'
static const uint8_t bmp_8_cyrillic_0435[] = {
0x60, 
0xE0, 
0x80, 
0x60, 
};

// U+0436 'ж'
static const uint8_t bmp_8_cyrillic_0436[] = {
0x54, 
0x54, 
0x38, 
0x54, 
};

// U+0437 'з'
static const uint8_t bmp_8_cyrillic_0437[] = {
0x60, 
0x70, 
0x10, 
0x60, 
};

// U+0438 'и'
static const uint8_t bmp_8_cyrillic_0438[] = {
0x90, 
0xB0, 
0xD0, 
0x90, 
};

// U+0439 'й'
static const uint8_t bmp_8_cyrillic_0439[] = {
0x60, 
0x90, 
0xB0, 
0xD0, 
0x90, 
};

// U+043A 'к'
static const uint8_t bmp_8_cyrillic_043A[] = {
0xA0, 
0xA0, 
0xC0, 
0xA0, 
};

// U+043B 'л'
static const uint8_t bmp_8_cyrillic_043B[] = {
0x70, 
0x50, 
0x50, 
0xD0, 
};

// U+043C 'м'
static const uint8_t bmp_8_cyrillic_043C[] = {
0x88, 
0xD8, 
0xD8, 
0xA8, 
};

// U+043D 'н'
static const uint8_t bmp_8_cyrillic_043D[] = {
0x90, 
0x90, 
0xF0, 
0x90, 
};

// U+043E 'о'
static const uint8_t bmp_8_cyrillic_043E[] = {
0x60, 
0x90, 
0x90, 
0x60, 
};

// U+043F 'п'
static const uint8_t bmp_8_cyrillic_043F[] = {
0xF0, 
0x90, 
0x90, 
0x90, 
};

// U+0440 'р'
static const uint8_t bmp_8_cyrillic_0440[] = {
0xE0, 
0x90, 
0x90, 
0xE0, 
0x80, 
};

// U+0441 'с'
static const uint8_t bmp_8_cyrillic_0441[] = {
0x60, 
0x90, 
0x90, 
0x60, 
};

// U+0442 'т'
static const uint8_t bmp_8_cyrillic_0442[] = {
0xE0, 
0x40, 
0x40, 
0x40, 
};

// U+0443 'у'
static const uint8_t bmp_8_cyrillic_0443[] = {
0x90, 
0x60, 
0x60, 
0x40, 
0xC0, 
};

// U+0444 'ф'
static const uint8_t bmp_8_cyrillic_0444[] = {
0x20, 
0x20, 
0xF8, 
0xA8, 
0xA8, 
0xF8, 
0x20, 
};

// U+0445 'х'
static const uint8_t bmp_8_cyrillic_0445[] = {
0xD0, 
0x60, 
0x60, 
0x90, 
};

// U+0446 'ц'
static const uint8_t bmp_8_cyrillic_0446[] = {
0x90, 
0x90, 
0x90, 
0xF0, 
0x10, 
};

// U+0447 'ч'
static const uint8_t bmp_8_cyrillic_0447[] = {
0x90, 
0x90, 
0xF0, 
0x10, 
};

// U+0448 'ш'
static const uint8_t bmp_8_cyrillic_0448[] = {
0xA8, 
0xA8, 
0xA8, 
0xF8, 
};

// U+0449 'щ'
static const uint8_t bmp_8_cyrillic_0449[] = {
0xA4, 
0xA4, 
0xA4, 
0xFC, 
0x04, 
};

// U+044A 'ъ'
static const uint8_t bmp_8_cyrillic_044A[] = {
0xC0, 
0x70, 
0x48, 
0x70, 
};

// U+044B 'ы'
static const uint8_t bmp_8_cyrillic_044B[] = {
0x88, 
0xE8, 
0xA8, 
0xE8, 
};

// U+044C 'ь'
static const uint8_t bmp_8_cyrillic_044C[] = {
0x80, 
0xE0, 
0x90, 
0xE0, 
};

// U+044D 'э'
static const uint8_t bmp_8_cyrillic_044D[] = {
0x60, 
0x30, 
0x90, 
0x60, 
};

// U+044E 'ю'
static const uint8_t bmp_8_cyrillic_044E[] = {
0xB8, 
0xA4, 
0xE4, 
0xB8, 
};

// U+044F 'я'
static const uint8_t bmp_8_cyrillic_044F[] = {
0x70, 
0x90, 
0x70, 
0x50, 
};

// U+0451 'ё'
static const uint8_t bmp_8_cyrillic_0451[] = {
0x50, 
0x60, 
0xE0, 
0x80, 
0x60, 
};

// U+00B0 '°'
static const uint8_t bmp_8_cyrillic_00B0[] = {
0x40, 
0x40, 
};

// U+00B5 'µ'
static const uint8_t bmp_8_cyrillic_00B5[] = {
0x90, 
0x90, 
0x90, 
0xF0, 
0x80, 
};

// U+00B2 '²'
static const uint8_t bmp_8_cyrillic_00B2[] = {
0xC0, 
0x40, 
0xE0, 
};

// U+00B3 '³'
static const uint8_t bmp_8_cyrillic_00B3[] = {
0xC0, 
0x40, 
0xE0, 
};

// U+2082 '₂'
static const uint8_t bmp_8_cyrillic_2082[] = {
0xC0, 
0x40, 
0xE0, 
};

static const Glyph glyphs_8_cyrillic[] = {
  { 0x0401, 4, 7, 0, bmp_8_cyrillic_0401 }, // 'Ё'
  { 0x0410, 5, 5, 2, bmp_8_cyrillic_0410 }, // 'А'
  { 0x0411, 4, 5, 2, bmp_8_cyrillic_0411 }, // 'Б'
  { 0x0412, 4, 5, 2, bmp_8_cyrillic_0412 }, // 'В'
  { 0x0413, 4, 5, 2, bmp_8_cyrillic_0413 }, // 'Г'
  { 0x0414, 6, 7, 2, bmp_8_cyrillic_0414 }, // 'Д'
  { 0x0415, 4, 5, 2, bmp_8_cyrillic_0415 }, // 'Е'
  { 0x0416, 7, 5, 2, bmp_8_cyrillic_0416 }, // 'Ж'
  { 0x0417, 4, 5, 2, bmp_8_cyrillic_0417 }, // 'З'
  { 0x0418, 4, 5, 2, bmp_8_cyrillic_0418 }, // 'И'
  { 0x0419, 4, 7, 0, bmp_8_cyrillic_0419 }, // 'Й'
  { 0x041A, 5, 5, 2, bmp_8_cyrillic_041A }, // 'К'
  { 0x041B, 5, 5, 2, bmp_8_cyrillic_041B }, // 'Л'
  { 0x041C, 6, 5, 2, bmp_8_cyrillic_041C }, // 'М'
  { 0x041D, 5, 5, 2, bmp_8_cyrillic_041D }, // 'Н'
  { 0x041E, 5, 5, 2, bmp_8_cyrillic_041E }, // 'О'
  { 0x041F, 5, 5, 2, bmp_8_cyrillic_041F }, // 'П'
  { 0x0420, 4, 5, 2, bmp_8_cyrillic_0420 }, // 'Р'
  { 0x0421, 5, 5, 2, bmp_8_cyrillic_0421 }, // 'С'
  { 0x0422, 5, 5, 2, bmp_8_cyrillic_0422 }, // 'Т'
  { 0x0423, 5, 5, 2, bmp_8_cyrillic_0423 }, // 'У'
  { 0x0424, 5, 6, 2, bmp_8_cyrillic_0424 }, // 'Ф'
  { 0x0425, 5, 5, 2, bmp_8_cyrillic_0425 }, // 'Х'
  { 0x0426, 5, 7, 2, bmp_8_cyrillic_0426 }, // 'Ц'
  { 0x0427, 4, 5, 2, bmp_8_cyrillic_0427 }, // 'Ч'
  { 0x0428, 7, 5, 2, bmp_8_cyrillic_0428 }, // 'Ш'
  { 0x0429, 7, 7, 2, bmp_8_cyrillic_0429 }, // 'Щ'
  { 0x042A, 6, 5, 2, bmp_8_cyrillic_042A }, // 'Ъ'
  { 0x042B, 6, 5, 2, bmp_8_cyrillic_042B }, // 'Ы'
  { 0x042C, 4, 5, 2, bmp_8_cyrillic_042C }, // 'Ь'
  { 0x042D, 5, 5, 2, bmp_8_cyrillic_042D }, // 'Э'
  { 0x042E, 6, 5, 2, bmp_8_cyrillic_042E }, // 'Ю'
  { 0x042F, 4, 5, 2, bmp_8_cyrillic_042F }, // 'Я'
  { 0x0430, 4, 4, 3, bmp_8_cyrillic_0430 }, // 'а'
  { 0x0431, 4, 6, 1, bmp_8_cyrillic_0431 }, // 'б'
  { 0x0432, 4, 4, 3, bmp_8_cyrillic_0432 }, // 'в'
  { 0x0433, 3, 4, 3, bmp_8_cyrillic_0433 }, // 'г'
  { 0x0434, 5, 5, 3, bmp_8_cyrillic_0434 }, // 'д'
  { 0x0435, 4, 4, 3, bmp_8_cyrillic_0435 }, // 'е'
  { 0x0436, 7, 4, 3, bmp_8_cyrillic_0436 }, // 'ж'
  { 0x0437, 4, 4, 3, bmp_8_cyrillic_0437 }, // 'з'
  { 0x0438, 4, 4, 3, bmp_8_cyrillic_0438 }, // 'и'
  { 0x0439, 4, 5, 2, bmp_8_cyrillic_0439 }, // 'й'
  { 0x043A, 4, 4, 3, bmp_8_cyrillic_043A }, // 'к'
  { 0x043B, 4, 4, 3, bmp_8_cyrillic_043B }, // 'л'
  { 0x043C, 5, 4, 3, bmp_8_cyrillic_043C }, // 'м'
  { 0x043D, 4, 4, 3, bmp_8_cyrillic_043D }, // 'н'
  { 0x043E, 4, 4, 3, bmp_8_cyrillic_043E }, // 'о'
  { 0x043F, 4, 4, 3, bmp_8_cyrillic_043F }, // 'п'
  { 0x0440, 4, 5, 3, bmp_8_cyrillic_0440 }, // 'р'
  { 0x0441, 4, 4, 3, bmp_8_cyrillic_0441 }, // 'с'
  { 0x0442, 3, 4, 3, bmp_8_cyrillic_0442 }, // 'т'
  { 0x0443, 4, 5, 3, bmp_8_cyrillic_0443 }, // 'у'
  { 0x0444, 5, 7, 1, bmp_8_cyrillic_0444 }, // 'ф'
  { 0x0445, 4, 4, 3, bmp_8_cyrillic_0445 }, // 'х'
  { 0x0446, 4, 5, 3, bmp_8_cyrillic_0446 }, // 'ц'
  { 0x0447, 4, 4, 3, bmp_8_cyrillic_0447 }, // 'ч'
  { 0x0448, 5, 4, 3, bmp_8_cyrillic_0448 }, // 'ш'
  { 0x0449, 6, 5, 3, bmp_8_cyrillic_0449 }, // 'щ'
  { 0x044A, 5, 4, 3, bmp_8_cyrillic_044A }, // 'ъ'
  { 0x044B, 5, 4, 3, bmp_8_cyrillic_044B }, // 'ы'
  { 0x044C, 4, 4, 3, bmp_8_cyrillic_044C }, // 'ь'
  { 0x044D, 4, 4, 3, bmp_8_cyrillic_044D }, // 'э'
  { 0x044E, 6, 4, 3, bmp_8_cyrillic_044E }, // 'ю'
  { 0x044F, 4, 4, 3, bmp_8_cyrillic_044F }, // 'я'
  { 0x0451, 4, 5, 2, bmp_8_cyrillic_0451 }, // 'ё'
  { 0x00B0, 3, 2, 2, bmp_8_cyrillic_00B0 }, // '°'
  { 0x00B5, 4, 5, 3, bmp_8_cyrillic_00B5 }, // 'µ'
  { 0x00B2, 3, 3, 1, bmp_8_cyrillic_00B2 }, // '²'
  { 0x00B3, 3, 3, 1, bmp_8_cyrillic_00B3 }, // '³'
  { 0x2082, 3, 3, 5, bmp_8_cyrillic_2082 }, // '₂'
};

const Font font_8_cyrillic = {
  71, 8, glyphs_8_cyrillic
};
