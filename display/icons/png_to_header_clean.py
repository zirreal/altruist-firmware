#!/usr/bin/env python3
# Scalar vector icons to Adafruit GFX format helper for esp32-weather-epd.
# Copyright (C) 2022-2024  Luke Marzen
# Clean version without dithering for less grain
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import getopt
import os.path
import sys
from PIL import Image

BITES_PER_LINE = 12
BITS_PER_BITE = 8
THRESHOLD = 127

try:
    opts, args = getopt.getopt(sys.argv[1:],"hi:o:t:",["inputfile=","outputfile=","threshold="])
except getopt.GetoptError:
    print('png_to_header_clean.py -i <inputfile> -o <outputfile> [-t <threshold>]')
    sys.exit(2)

threshold = THRESHOLD
for opt, arg in opts:
    if opt == '-h':
        print('png_to_header_clean.py -i <inputfile> -o <outputfile> [-t <threshold>]')
        print('  -t, --threshold: Threshold value (0-255, default: 127)')
        sys.exit()
    elif opt in ("-i", "--inputfile"):
        inputfile = arg
    elif opt in ("-o", "--outputfile"):
        outputfile = arg
    elif opt in ("-t", "--threshold"):
        threshold = int(arg)

try: inputfile
except NameError:
    print("Error: inputfile is a required parameter. See usage -h.")
    exit()
try: outputfile
except NameError:
    print("Error: outputfile is a required parameter. See usage -h")
    exit()

src_image = Image.open(inputfile)
# Converts the image to grayscale
src_g = src_image.convert('L')
# Creates a list of the pixel values
pixels = list(src_g.getdata())

f = open(outputfile, "w")
var = os.path.basename(outputfile)
var = var.rsplit('.h',1)[0]

width, height = src_image.size
f.write("// " + str(width) + " x " + str(height) + "\n")
f.write("const unsigned char " + var + "[] PROGMEM = {\n ")

bit_cnt = 1
transcribed_width = 1
tmp_bite = 0
n = len(pixels)
line_width = 0

for i in range(n):
    line_width += 1
    if (pixels[i] > threshold):
        tmp_bite |= 1
    if (line_width == width):
        tmp_bite <<= BITS_PER_BITE - bit_cnt
        line_width = 0
        bit_cnt = 8
    # if there is still more room in the current byte, shift the bits to make room for the next new bit
    if (bit_cnt < BITS_PER_BITE and i != n - 1):
        tmp_bite <<= 1
        bit_cnt += 1
    # else byte must be complete, write the byte to file
    elif (i != n - 1):
        f.write(" " + "0x{:02x}".format(tmp_bite) + ",")
        if (transcribed_width == BITES_PER_LINE):
            f.write("\n ")
            transcribed_width = 1
        else:
            transcribed_width += 1
        tmp_bite = 0
        bit_cnt = 1

tmp_bite <<= BITS_PER_BITE - bit_cnt
f.write(" " + "0x{:02x}".format(tmp_bite) + "\n};")
f.close()





