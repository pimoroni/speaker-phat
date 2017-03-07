#!/usr/bin/env python

import math
import time
import sys

sys.path.append('../python/')

import speakerphat


speed = 4

for i in range(1000):
    offset = int((math.sin(time.time() * speed) * 5) + 5)

    speakerphat.clear()
    speakerphat.set_led(offset,255)
    speakerphat.show()
