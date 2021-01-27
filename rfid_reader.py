# SPDX-FileCopyrightText: 2017 Tony DiCola for Adafruit Industries
# SPDX-FileCopyrightText: 2017 James DeVito for Adafruit Industries
# SPDX-License-Identifier: MIT

# This example is for use on (Linux) computers that are using CPython with
# Adafruit Blinka to support CircuitPython libraries. CircuitPython does
# not support PIL/pillow (python imaging library)!

import time
import asyncio
from evdev import InputDevice, categorize, ecodes

# connect to the RFID reader
dev = InputDevice('/dev/input/event0')

from board import SCL, SDA
import busio
from PIL import Image, ImageDraw, ImageFont
import adafruit_ssd1306

# Create the I2C interface.
i2c = busio.I2C(SCL, SDA)

# Create the SSD1306 OLED class.
# The first two parameters are the pixel width and pixel height.  Change these
# to the right size for your display!
disp = adafruit_ssd1306.SSD1306_I2C(128, 32, i2c)

# Clear display.
disp.fill(0)
disp.show()

# Create blank image for drawing.
# Make sure to create image with mode '1' for 1-bit color.
width = disp.width
height = disp.height
image = Image.new("1", (width, height))

# Get drawing object to draw on image.
draw = ImageDraw.Draw(image)

# Draw a black filled box to clear the image.
draw.rectangle((0, 0, width, height), outline=0, fill=0)

# Draw some shapes.
# First define some constants to allow easy resizing of shapes.
padding = -2
top = padding
bottom = height - padding
# Move left to right keeping track of the current x position for drawing shapes.
x = 0


# Load default font.
#font = ImageFont.load_default()

# Alternatively load a TTF font.  Make sure the .ttf font file is in the
# same directory as the python script!
# Some other nice fonts to try: http://www.dafont.com/bitmap.php
# font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 9)
font = ImageFont.truetype('./tt0246m_.ttf', 16)


async def helper(dev):
    tagId = ""
    tagIsDone = False
    # Draw a black filled box to clear the image.
    draw.rectangle((0, 0, width, height), outline=0, fill=0)
    
    async for ev in dev.async_read_loop():
        if tagIsDone == True:
            tagId = ""
            tagIsDone = False
        if ev.type == ecodes.EV_KEY and ev.value == 0: # numbers
            if ev.code <= 11:
                #print(ev.code-1)
                if ev.code == 11:
                    tagId = tagId + "0"
                else:
                    tagId = tagId + str(ev.code-1)
        if ev.code == 28: # enter key up
            print(tagId)
            # Draw a black filled box to clear the image.
            draw.rectangle((0, 0, width, height), outline=0, fill=0)
            draw.text((x, top + 0), "Tag: " + tagId, font=font, fill=255)
            # Display image.
            disp.image(image)
            disp.show()
            tagIsDone = True

loop = asyncio.get_event_loop()
loop.run_until_complete(helper(dev))

