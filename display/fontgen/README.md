# Display bitmap fonts (ASCII + Cyrillic)

Generate C bitmap fonts for the Insight display.

## Usage

From repo root:

```bash
python display/fontgen/ttf_to_bitmap.py
```

Then rebuild firmware (e.g. `pio run -e esp32c6_inside_ru`).

## Font choice (e-ink / low-res)

1. Download any font you want.
2. Put it in `display/fontgen/` (same folder as this README).
3. Run the script; it will pick DejaVu/Noto if present, otherwise `font.ttf`.
4. Or use a specific file: `FONT_FILE=MyFont.ttf python display/fontgen/ttf_to_bitmap.py`.

Output goes to `display/fontgen/out/` (font_8_ascii.c, font_16_cyrillic.c, etc.).
