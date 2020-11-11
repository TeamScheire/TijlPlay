#!/usr/bin/env python2
# -*- coding: utf-8 -*-
"""
Created on Sun Aug  4 10:15:29 2019

@author: benny
"""

FILENAME1 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/train_toewalking.csv'
FILENAME2 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/train_walking.csv'
#FILENAME1 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/train_toewalking02.csv'
#FILENAME2 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/train_walking02.csv'

ROLL_AVG_WINDOW = 8

import csv
import numpy as np
import matplotlib.pyplot as plt

def do_read(filename):
    timesec = []
    accel = []
    accel_forward = []
    accel_z = []
    accel_sideways = []

    with open(filename) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        line_count = 0
        for row in csv_reader:
            timesec += [row[0]]
            accel += [row[1]]
            accel_forward += [row[2]]
            accel_z += [row[3]]
            accel_sideways += [row[4]]
            line_count += 1
        print(f'Processed {line_count} lines.')

    #convert to numpy arrays
    timesec = np.array(timesec, float)
    accel = np.array(accel, float)
    accel_forward = np.array(accel_forward, float)
    accel_z = np.array(accel_z, float)
    accel_sideways = np.array(accel_sideways, float)

    # offset time with start time
    timesec = timesec - timesec[0]

    return timesec, accel, accel_forward, accel_z, accel_sideways

# make rolling average of accelleration
def rolling_window(a, window):
    shape = a.shape[:-1] + (a.shape[-1] - window + 1, window)
    strides = a.strides + (a.strides[-1],)
    return np.lib.stride_tricks.as_strided(a, shape=shape, strides=strides)

toewalk_data = do_read(FILENAME1)
walk_data = do_read(FILENAME2)

accel_toewalk_mean = np.mean(rolling_window(toewalk_data[1], ROLL_AVG_WINDOW), 1) # rolling average of the mean
accel_walk_mean = np.mean(rolling_window(walk_data[1], ROLL_AVG_WINDOW), 1) # rolling average of the mean

plt.figure()
plt.plot(toewalk_data[0][:len(accel_toewalk_mean)], accel_toewalk_mean, 'r-')
plt.plot(walk_data[0][:len(accel_walk_mean)], accel_walk_mean, 'b-')
#plt.show()

#algorithm to determine peak to peak and peak width
def process_peaks(csv_data):
    window = ROLL_AVG_WINDOW
    window_values = np.zeros(window, float)
    count = 0
    accel_avg = 0.
    peak_resolution_accel = 30
    peak_width_resolution_accel = 50
    peaks_sec = [csv_data[0][0], csv_data[0][0], csv_data[0][0] ]
    peaks = [-1, -1, -1]
    peaks_width = [0, 0, 0]
    current_peak = 0
    nr_peaks = 3
    valleys_sec = [csv_data[0][0], csv_data[0][0], csv_data[0][0] ]
    valleys = [2000, 2000, 2000]
    valleys_width = [0, 0, 0]
    current_valley = 0
    nr_valleys = 3
    peak_found = False
    valley_found = False

    amplitude_data = []
    wavelength_data = []

    prev_accel = csv_data[1][0]
    prevprev_accel = csv_data[1][0]

    for sec, accel in zip(csv_data[0], csv_data[1]):
        window_values[count] = accel
        count += 1
        count = count % window
        accel_avg = np.mean(window_values)
        if accel_avg > peaks[current_peak]:
            # going towards a peak
            peaks[current_peak] = accel_avg
            peaks_sec[current_peak] = sec
            if accel_avg > prev_accel and accel_avg > prevprev_accel:
                peak_found = True
        elif peak_found and accel_avg < peaks[current_peak] - peak_resolution_accel:
            # peak finished, go to next peak
            peaks_width[current_peak] = 0
            print ('New peak registered at', peaks_sec[current_peak], 'accel=', peaks[current_peak] )
            current_peak += 1
            current_peak = current_peak % nr_peaks
            peaks[current_peak] = accel_avg
            peaks_sec[current_peak] = sec
            peak_found = False
        elif peaks_width[current_peak-1] == 0 \
                and accel_avg < peaks[current_peak-1] - peak_width_resolution_accel \
                and sec - peaks_sec[current_peak-1] < 0.8:
            # we consider this the half peak width of the previous peak
            peaks_width[current_peak-1] = 2*(sec - peaks_sec[current_peak-1])
        elif sec - peaks_sec[current_peak] > 0.8:
            #force stop of peak
            print ('New peak FORCE registered at', peaks_sec[current_peak], 'accel=', peaks[current_peak] )
            peaks_width[current_peak] = 0
            current_peak += 1
            current_peak = current_peak % nr_peaks
            peaks[current_peak] = accel_avg
            peaks_sec[current_peak] = sec
            peak_found = False

        if accel_avg < valleys[current_valley]:
            # going towards a valley
            valleys[current_valley] = accel_avg
            valleys_sec[current_valley] = sec
            if accel_avg < prev_accel and accel_avg < prevprev_accel:
                valley_found = True
        elif valley_found and accel_avg > valleys[current_valley] + peak_resolution_accel:
            # valley finished, go to next valley
            print ('New valley registered at', valleys_sec[current_valley],
                   'accel=', valleys[current_valley])
            valleys_width[current_valley] = 0
            current_valley += 1
            current_valley = current_valley % nr_valleys
            valleys[current_valley] = accel_avg
            valleys_sec[current_valley] = sec
            valley_found = False
        elif valleys_width[current_valley-1] == 0 \
                and accel_avg > valleys[current_valley-1] + peak_width_resolution_accel \
                and sec - valleys_sec[current_valley-1] < 0.8:
            # we consider this the half valley width of the previous valley
            valleys_width[current_valley-1] = 2*(sec - valleys_sec[current_valley-1])
        elif sec - valleys_sec[current_valley] > 0.8:
            #force stop of valley
            print ('New valley FORCE registered at', valleys_sec[current_valley],
                   'accel=', valleys[current_valley])
            valleys_width[current_valley] = 0
            current_valley += 1
            current_valley = current_valley % nr_valleys
            valleys[current_valley] = accel_avg
            valleys_sec[current_valley] = sec
            valley_found = False

        amplitude_data += [peaks[current_peak-1] - valleys[current_valley-1]]
        wavelength_data += [peaks_width[current_peak-1]]

        prevprev_accel = prev_accel
        prev_accel = accel_avg

    return (np.array(amplitude_data, float), np.array(wavelength_data, float))

toewalk_amp, toewalk_wavelength = process_peaks(toewalk_data)
walk_amp, walk_wavelength = process_peaks(walk_data)

plt.figure()
plt.plot(toewalk_data[0][:len(toewalk_amp)], toewalk_amp, 'r-')
plt.plot(walk_data[0][:len(walk_amp)], walk_amp, 'b-')

plt.figure()
plt.plot(toewalk_data[0][:len(toewalk_wavelength)], toewalk_wavelength, 'r-')
plt.plot(walk_data[0][:len(walk_wavelength)], walk_wavelength, 'b-')

# rolling average over some 4 sec
amplitude_toewalk_mean = np.mean(rolling_window(toewalk_amp, 50), 1) # rolling average of the mean
amplitude_walk_mean = np.mean(rolling_window(walk_amp, 50), 1) # rolling average of the mean

plt.figure()
plt.plot(toewalk_data[0][:len(amplitude_toewalk_mean)], amplitude_toewalk_mean, 'r-')
plt.plot(walk_data[0][:len(amplitude_walk_mean)], amplitude_walk_mean, 'b-')

plt.show()