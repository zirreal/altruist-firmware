"""
Generate C bitmap fonts for display (ASCII + Cyrillic).
Run: python3 display/fontgen/ttf_to_bitmap.py
Use a different font: put DejaVuSans.ttf (or NotoSans-Regular.ttf) in this dir,
or set env FONT_FILE=DejaVuSans.ttf. DejaVu / Noto often look cleaner than Roboto at small e-ink sizes.
Roboto is the default one now.
"""
import freetype
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(SCRIPT_DIR, "out")

# Font: FONT_FILE env, or first file that exists (e-ink friendly first)
FONT_CANDIDATES = [
    os.environ.get("FONT_FILE"),
    "font.ttf",
    "NotoSans.ttf",
    "DejaVuSans.ttf",
]
FONT_FILE = None
for name in FONT_CANDIDATES:
    if name:
        path = os.path.join(SCRIPT_DIR, name) if not os.path.isabs(name) else name
        if os.path.isfile(path):
            FONT_FILE = path
            break
if not FONT_FILE:
    FONT_FILE = os.path.join(SCRIPT_DIR, "font.ttf")
if not os.path.isfile(FONT_FILE):
    raise SystemExit("No font found. Put DejaVuSans.ttf or font.ttf in " + SCRIPT_DIR)
print("Using font:", os.path.basename(FONT_FILE))

SIZES = [8, 12, 14, 16, 20, 24, 32, 36, 40, 48]

EXTRA_SYMBOLS = [
    0x00B0,  # degree sign: °
    0x00B5,  # micro sign: µ
    0x00B2,  # superscript two: ²
    0x00B3,  # superscript three: ³
    0x2082,  # subscript two: ₂
]

LANGUAGES = {
    "ascii": list(range(0x20, 0x7F)) + EXTRA_SYMBOLS,
    "cyrillic": (
        [0x0401] +                      # Ё
        list(range(0x0410, 0x0430)) +   # А–Я
        list(range(0x0430, 0x0450)) +   # а–я
        [0x0451] +                      # ё
        EXTRA_SYMBOLS
    )
}

os.makedirs(OUT_DIR, exist_ok=True)

def render(face, cp, size):
    # FORCE_AUTOHINT: grid-fit small glyphs and punctuation; TARGET_MONO: 1bpp
    face.load_char(chr(cp),
        freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO | freetype.FT_LOAD_FORCE_AUTOHINT)

    bmp = face.glyph.bitmap
    w = bmp.width
    bitmap_top = face.glyph.bitmap_top  # distance from baseline to top of bitmap
    bh = bmp.rows  # actual bitmap height (e.g. 3 for period, 16 for 'A')

    rows = []
    for y in range(bh):
        row = 0
        for x in range(w):
            if y < bmp.rows and x < bmp.width:
                byte = bmp.buffer[y * bmp.pitch + (x >> 3)]
                bit = (byte >> (7 - (x & 7))) & 1
                row |= bit << (w - 1 - x)
        rows.append(row)
    if bmp.pitch < 0:
        rows.reverse()
    # top = y-offset from line top to bitmap top (so punctuation sits at baseline, not top)
    top = max(0, (size - 1) - bitmap_top)
    return rows, w, bh, top

for size in SIZES:
    face = freetype.Face(FONT_FILE)
    face.set_pixel_sizes(0, size)

    for lang, cps in LANGUAGES.items():
        c_name = f"font_{size}_{lang}.c"
        h_name = f"font_{size}_{lang}.h"

        with open(os.path.join(OUT_DIR, c_name), "w") as c, open(os.path.join(OUT_DIR, h_name), "w") as h:
            guard = f"FONT_{size}_{lang.upper()}_H"
            h.write(f"#ifndef {guard}\n#define {guard}\n\n")
            h.write("#include <stdint.h>\n\n")
            h.write("typedef struct {\n")
            h.write("  uint32_t codepoint;\n")
            h.write("  uint16_t width;\n")
            h.write("  uint16_t height;\n")
            h.write("  uint16_t top;\n")
            h.write("  const uint8_t* bitmap;\n")
            h.write("} Glyph;\n\n")
            h.write("typedef struct {\n")
            h.write("  uint16_t count;\n")
            h.write("  uint16_t line_height;\n")
            h.write("  const Glyph* glyphs;\n")
            h.write("} Font;\n\n")
            h.write(f"extern const Font font_{size}_{lang};\n\n")
            h.write("#endif\n")

            c.write(f'#include "{h_name}"\n\n')

            glyph_entries = []

            for cp in cps:
                rows, w, bh, top = render(face, cp, size)
                bmp_name = f"bmp_{size}_{lang}_{cp:04X}"

                c.write(f"// U+{cp:04X} '{chr(cp)}'\n")
                c.write(f"static const uint8_t {bmp_name}[] = {{\n")
                bytes_per_row = (w + 7) // 8
                for r in rows:
                    for i in range(bytes_per_row):
                        start_bit = w - 8 * (i + 1)
                        if start_bit >= 0:
                            c.write(f"0x{(r >> start_bit) & 0xFF:02X}, ")
                        else:
                            c.write(f"0x{((r << (-start_bit)) & 0xFF):02X}, ")
                    c.write("\n")
                c.write("};\n\n")

                glyph_entries.append((cp, w, bh, top, bmp_name))

            c.write(f"static const Glyph glyphs_{size}_{lang}[] = {{\n")
            for cp, w, bh, top, bmp in glyph_entries:
                c.write(
                    f"  {{ 0x{cp:04X}, {w}, {bh}, {top}, {bmp} }}, // '{chr(cp)}'\n"
                )
            c.write("};\n\n")

            c.write(f"const Font font_{size}_{lang} = {{\n")
            c.write(f"  {len(glyph_entries)}, {size}, glyphs_{size}_{lang}\n")
            c.write("};\n")

        print(f"Generated: {c_name}")
