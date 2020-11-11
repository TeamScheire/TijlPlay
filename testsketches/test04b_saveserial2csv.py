# test04 code prints to serial the following:
#    print (sec, accel, peaks[current_peak-1], peaks_sec[current_peak-1],
#           valleys[current_valley-1], valleys_sec[current_valley-1], amplitude_avg)
# we capture this here and save
import serial
import time
import csv

# Typing dmesg | tail will shows you which /dev/ node the micro:bit was assigned
port = '/dev/ttyACM0'

# Filename to save to
filename = "test04_data.csv"

import os.path
if os.path.isfile(filename):
    print("File {0} exists. Rename!".format(filename))
    raise Exception("File {0} exists. Rename!".format(filename))


#ser = serial.Serial(port)
ser = serial.Serial(port=port,
                         baudrate=115200,
                         bytesize=serial.EIGHTBITS,
                         parity=serial.PARITY_NONE,
                         stopbits=serial.STOPBITS_ONE,
                         timeout=1)

eol = b'\r'
leneol = len(eol)

def readline():
    line = bytearray()
    while True:
        c = ser.read(1)
        if c:
            line += c
            if line[-leneol:] == eol:
                break
        else:
            break
    return bytes(line)

ser.flushInput()

while True:
    try:
        ser_bytes = readline()[2:-2].strip()
        ser_bytes = ser_bytes.decode("utf-8").split(',')
        if len(ser_bytes) == 10:
            decoded_bytes = [float(x) for x in ser_bytes]
            #print (len(decoded_bytes))
            if decoded_bytes[0] != 0:
                decoded_bytes.insert(0, time.time())
                #print(ser_bytes, decoded_bytes)
                with open(filename,"a") as f:
                    writer = csv.writer(f,delimiter=",")
                    writer.writerow(decoded_bytes)
    except ValueError:
        print("Cannot convert to floats", ser_bytes)
    except:
        print("Keyboard Interrupt")
        break
