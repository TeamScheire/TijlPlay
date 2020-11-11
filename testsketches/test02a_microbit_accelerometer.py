from microbit import *
import math

# MicroPython is able to recognise the following gestures:
#  up, down, left, right, face up, face down, freefall, 3g, 6g, 8g, shake.
while True:
    x = float(accelerometer.get_x())
    y = float(accelerometer.get_y())
    z = float(accelerometer.get_z())
    acceleration = math.sqrt(x**2 + y**2 + z**2)
    # print("acceleration", acceleration)

    # now check gesture
    gesture = accelerometer.current_gesture()
    if gesture == "face up":
        display.show(Image.HAPPY)
    else:
        display.show(Image.ANGRY)

    # now we plot some data, see in mu-editor in the plotter and REPL output
    # vals = accelerometer.get_values()
    print((acceleration, x, y, z))
    sleep(20)