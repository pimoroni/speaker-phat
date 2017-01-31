#!/usr/bin/env python

import math
import time

import speakerphat


speed = 4

while True:
    offset = int((math.sin(time.time() * speed) * 5) + 5)

    speakerphat.clear()
    speakerphat.set_led(offset,255)
    speakerphat.show()
