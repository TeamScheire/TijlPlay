import sys
import random
import time
import numpy as np
import csv

# ID of your Microbit
BLUETOOTH_ID_MICROBIT = "D1:F7:E5:B5:11:88"

# Filename to save to
filename = "test03_data.csv"

from bluepy import btle

def main(num_iterations=sys.maxsize):
    print('Connecting to Microbit ...')
    p = btle.Peripheral(BLUETOOTH_ID_MICROBIT, btle.ADDR_TYPE_RANDOM)

    p.setSecurityLevel("medium")
    print('Connected !')

    print('Subscribing to Microbit acceleration data service ...')

    svc = p.getServiceByUUID("e95d0753-251d-470a-a062-fa1922dfa9a8")
    ch = svc.getCharacteristics("e95dca4b-251d-470a-a062-fa1922dfa9a8")[0]
    chper = svc.getCharacteristics("e95dfb24-251d-470a-a062-fa1922dfa9a8")[0]

    print('Subscribed !\n Characteristic:', ch)

    CCCD_UUID = 0x2902

    ch_cccd=ch.getDescriptors(forUUID=CCCD_UUID)[0]

    print(' Descriptors:', ch_cccd)

    # write to indicate we will read out data
    ch_cccd.write(b"\x00\x00", False)

    print_output = np.empty(5, float)

    while True:
        coord = np.fromstring(ch.read(), dtype=np.int16, count=3)
        #print (coord)
        print_output[0] = time.time() # time in seconds
        print_output[2:] = coord[:]
        print_output[1] = np.linalg.norm(coord) # total acceleration
        with open(filename,"a") as f:
            writer = csv.writer(f, delimiter=",")
            writer.writerow(print_output)

if __name__ == '__main__':
    while True:
        try:
            main()
        except btle.BTLEDisconnectError:
            print("Device disconnected, waiting 5 sec to retry connection")
            time.sleep(5)   # Delays for 5 seconds.
        except KeyboardInterrupt:
            print("Keyboard Interrupt. Stopping code")
            break