# -*- coding: utf-8 -*-
"""
Created on Sun Aug  4 10:15:29 2019

@author: benny

# process the serial data of test04 test script which captured Microbit data
"""

#FILENAME1 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/test04_toewalking_diff_speeds01.csv'
#FILENAME2 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/test04_walking_diff_speeds01.csv'
FILENAME1 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/test04_toewalking_01.csv'
FILENAME2 = '/media/benny/hd2/datahd2/git/smartshoe/testsketches/test04_walking_01.csv'
#FILENAME1 = '/home/mcciocci/git/smartshoe/testsketches/test_04_data_toewalkingtijl16092019.csv'
#FILENAME2 = '/home/mcciocci/git/smartshoe/testsketches/test_04_data_walkingtijl16092019.csv'
#old values
#CUTOFF_X = [  0, 29.5, 31, 36, 100, 110, 125, 130, 145]
#CUTOFF_Y = [250, 250, 240, 240, 365, 375, 460, 610, 811]
#value Cri
#CUTOFF_X = [  0, 100, 110, 125, 130, 145]
#CUTOFF_Y = [200, 365, 375, 460, 610, 811]
#value T
CUTOFF_X = [ 0, 60, 118, 145, 155, 165]
CUTOFF_Y = [50, 50, 240, 270, 400, 811]

SKIP = 200

ROLL_AVG_WINDOW = 8

CORRECT_WRONG_STEPSPERMIN = False

# CSV structure is (time_sec_recording, sec, accel, current_peak,
#   stepspermin, peaks[current_peak-1], peaks_sec[current_peak-1],
#   valleys[current_valley-1], valleys_sec[current_valley-1],
#   amplitude_avg, 0

import csv
import numpy as np
import matplotlib.pyplot as plt

def do_read(filename, skip=0):
    timesec = []
    accel = []
    stepspermin = []
    amplitude_avg = []

    with open(filename) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        line_count = 0
        for row in csv_reader:
            timesec += [row[1].replace(',','.')]
            accel += [row[2].replace(',','.')]
            stepspermin += [row[4].replace(',','.')]
            amplitude_avg += [row[9].replace(',','.')]
            line_count += 1
            if line_count > 1:
                if abs(float(timesec[-1])-float(timesec[-2])) > 1 \
                    or float(timesec[-1]) == float(timesec[-2]):
                    timesec = timesec[:-1]
                    accel = accel[:-1]
                    stepspermin = stepspermin[:-1]
                    amplitude_avg = amplitude_avg[:-1]
        print(f'Processed {line_count} lines.')

    #convert to numpy arrays
    timesec = np.array(timesec, float)
    accel = np.array(accel, float)
    stepspermin = np.array(stepspermin, float)
    if CORRECT_WRONG_STEPSPERMIN:
        stepspermin = 60/stepspermin*60
    amplitude_avg = np.array(amplitude_avg, float)

    # offset time with start time
    timesec = timesec - timesec[skip]

    return timesec[skip:], accel[skip:], stepspermin[skip:], amplitude_avg[skip:]

# make rolling average of accelleration
def rolling_window(a, window):
    shape = a.shape[:-1] + (a.shape[-1] - window + 1, window)
    strides = a.strides + (a.strides[-1],)
    return np.lib.stride_tricks.as_strided(a, shape=shape, strides=strides)

toewalk_data = do_read(FILENAME1, skip=SKIP)
walk_data = do_read(FILENAME2, skip=SKIP)

sec_toewalk_mean = np.mean(rolling_window(toewalk_data[0], ROLL_AVG_WINDOW), 1) # rolling average of the mean
accel_toewalk_mean = np.mean(rolling_window(toewalk_data[1], ROLL_AVG_WINDOW), 1) # rolling average of the mean
sec_walk_mean = np.mean(rolling_window(walk_data[0], ROLL_AVG_WINDOW), 1) # rolling average of the mean
accel_walk_mean = np.mean(rolling_window(walk_data[1], ROLL_AVG_WINDOW), 1) # rolling average of the mean

plt.figure()
plt.plot(toewalk_data[0][:len(accel_toewalk_mean)], accel_toewalk_mean, 'r-')
plt.plot(walk_data[0][:len(accel_walk_mean)], accel_walk_mean, 'b-')
plt.title(f'Mean accel data over window of {ROLL_AVG_WINDOW}')
#plt.show()

#algorithm to determine peak to peak and peak width
def process_peaks(csv_data):
    window = ROLL_AVG_WINDOW
    window_values = np.zeros(window, float)
    count = 0
    accel_avg = 0.
    peak_resolution_accel = 50   # TODO SHOULD THIS NOT BE HIGHER
    peak_width_resolution_accel = 100
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
        if accel_avg > peaks[current_peak] and accel_avg > prev_accel: # TODO ADD accel > prev_accel
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
            peaks[current_peak] = 0
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
            peaks[current_peak] = 0
            peaks_sec[current_peak] = sec
            peak_found = False

        if accel_avg < valleys[current_valley] and accel_avg < prev_accel:
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
            valleys[current_valley] = 3000
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
            valleys[current_valley] = 3000
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
plt.plot(toewalk_data[0][:len(toewalk_amp)], toewalk_amp, 'r-', label="toewalk")
plt.plot(walk_data[0][:len(walk_amp)], walk_amp, 'b-', label="walk")
plt.legend(loc='upper left')
plt.title('Amplitude shocks - as in Microbit computed')

# rolling average over some 4 sec
amplitude_toewalk_mean = np.mean(rolling_window(toewalk_amp, 50), 1) # rolling average of the mean
amplitude_walk_mean = np.mean(rolling_window(walk_amp, 50), 1) # rolling average of the mean

plt.figure()
plt.plot(toewalk_data[0][:len(amplitude_toewalk_mean)], amplitude_toewalk_mean, 'r-', label="toewalk")
plt.plot(walk_data[0][:len(amplitude_walk_mean)], amplitude_walk_mean, 'b-', label="walk")
plt.legend(loc='upper left')
plt.title('Average Amplitude shocks - as in Microbit computed')

#new output amplitude determined on microbit
plt.figure()
plt.plot(toewalk_data[0][:len(toewalk_data[3])], toewalk_data[3], 'r-', label="toewalk")
plt.plot(walk_data[0][:len(walk_data[3])], walk_data[3], 'b-', label="walk")
plt.legend(loc='upper left')
plt.title('Average Amplitude shocks MicroBit')

# new output. Avg amplitude versus the speed
plt.figure()
plt.plot(toewalk_data[2], toewalk_data[3], 'r.', label="toewalk")
plt.plot(walk_data[2], walk_data[3], 'b.', label="walk")
plt.legend(loc='upper left')
plt.xlabel("Steps per min")
plt.ylabel("Avg Amplitude shocks")
plt.title('Average Shocks versus Speed (momentaneous)')

#average the steps per minute over 2 sec = 20ms * 100
windowsteps = 100
steps_avg_toewalk = np.mean(rolling_window(toewalk_data[2], windowsteps), 1)
steps_avg_walk = np.mean(rolling_window(walk_data[2], windowsteps), 1)
amp_avg_toewalk = np.mean(rolling_window(toewalk_data[3], windowsteps), 1)
amp_avg_walk = np.mean(rolling_window(walk_data[3], windowsteps), 1)

plt.figure()
plt.scatter(steps_avg_toewalk,
         toewalk_data[3][int(windowsteps/2):int(len(steps_avg_toewalk)+windowsteps/2)],
         1, c="r", alpha=0.5, label="toewalk")
plt.scatter(steps_avg_walk,
         walk_data[3][int(windowsteps/2):int(len(steps_avg_walk)+windowsteps/2)],
         1, c="b", alpha=0.5, label="walk")
plt.plot(CUTOFF_X, CUTOFF_Y, 'k-')
plt.legend(loc='upper left')
plt.xlabel("Avg Steps per min")
plt.ylabel("Avg Amplitude shocks")
plt.title('Average Shocks versus Average Speed (2sec)')

#average the steps per minute over 4 sec = 20ms * 200
windowsteps = 200
steps_avg_toewalk = np.mean(rolling_window(toewalk_data[2], windowsteps), 1)
steps_avg_walk = np.mean(rolling_window(walk_data[2], windowsteps), 1)
amp_avg_toewalk = np.mean(rolling_window(toewalk_data[3], windowsteps), 1)
amp_avg_walk = np.mean(rolling_window(walk_data[3], windowsteps), 1)

plt.figure()
plt.scatter(steps_avg_toewalk,
         toewalk_data[3][int(windowsteps/2):int(len(steps_avg_toewalk)+windowsteps/2)],
         1, c="r", alpha=0.5, label="toewalk")
plt.scatter(steps_avg_walk,
         walk_data[3][int(windowsteps/2):int(len(steps_avg_walk)+windowsteps/2)],
         1, c="b", alpha=0.5, label="walk")
plt.plot(CUTOFF_X, CUTOFF_Y, 'k-')
plt.legend(loc='upper left')
plt.xlabel("Avg Steps per min")
plt.ylabel("Amplitude shocks")
plt.title('Current Shocks versus Average Speed (4sec)')


#bonus extra average the amplitude
windowsteps = 200
steps_avg_toewalk = np.mean(rolling_window(toewalk_data[2], windowsteps), 1)
steps_avg_walk = np.mean(rolling_window(walk_data[2], windowsteps), 1)
amp_avg_toewalk = np.mean(rolling_window(toewalk_data[3], windowsteps), 1)
amp_avg_walk = np.mean(rolling_window(walk_data[3], windowsteps), 1)

plt.figure()
plt.scatter(steps_avg_toewalk,
         amp_avg_toewalk,
         1, c="r", alpha=0.5, label="toewalk")
plt.scatter(steps_avg_walk,
         amp_avg_walk,
         1, c="b", alpha=0.5, label="walk")
plt.plot(CUTOFF_X, CUTOFF_Y, 'k-')
plt.legend(loc='upper left')
plt.xlabel("Avg Steps per min")
plt.ylabel("Avg Amplitude shocks")
plt.title('Average Shocks versus Average Speed (4sec)')

# =============================================================================
# # output data with TV
# TV_accel_toewalk = np.abs((toewalk_data[1][1:]-toewalk_data[1][:-1]))
# TV_accel_walk = np.abs((walk_data[1][1:]-walk_data[1][:-1]))
# deriv_accel_toewalk = (toewalk_data[1][1:]-toewalk_data[1][:-1])/(toewalk_data[0][1:]-toewalk_data[0][:-1])
# deriv_accel_walk = (walk_data[1][1:]-walk_data[1][:-1])/(walk_data[0][1:]-walk_data[0][:-1])
# TV_deriv_accel_toewalk = np.abs((deriv_accel_toewalk[1:]-deriv_accel_toewalk[:-1]))
# TV_deriv_accel_walk = np.abs((deriv_accel_walk[1:]-deriv_accel_walk[:-1]))
# # total over xx sec (1 sec = 50 frame window)
# windowsteps = 100
# sec_TV_toewalk = np.mean(rolling_window(toewalk_data[0][1:], windowsteps), 1)
# avg_TV_toewalk = np.mean(rolling_window(TV_accel_toewalk, windowsteps), 1)
# avg_TVTV_toewalk = np.mean(rolling_window(TV_deriv_accel_toewalk, windowsteps), 1)
# sec_TV_walk = np.mean(rolling_window(walk_data[0][1:], windowsteps), 1)
# avg_TV_walk = np.mean(rolling_window(TV_accel_walk, windowsteps), 1)
# avg_TVTV_walk = np.mean(rolling_window(TV_deriv_accel_walk, windowsteps), 1)
#
# steps_avg_toewalk = np.mean(rolling_window(toewalk_data[2][1:], windowsteps), 1)
# steps_avg_walk = np.mean(rolling_window(walk_data[2][1:], windowsteps), 1)
#
# plt.figure()
# plt.scatter(sec_TV_toewalk,
#          avg_TV_toewalk,
#          1, c="r", alpha=0.5, label="toewalk")
# plt.scatter(sec_TV_walk,
#          avg_TV_walk,
#          1, c="b", alpha=0.5, label="walk")
# plt.legend(loc='upper left')
# plt.xlabel("sec")
# plt.ylabel("Avg TV amplitude")
# plt.title('Average TV Shocks versus time')
#
# plt.figure()
# plt.scatter(sec_TV_toewalk[1:],
#          avg_TVTV_toewalk,
#          1, c="r", alpha=0.5, label="toewalk")
# plt.scatter(sec_TV_walk[1:],
#          avg_TVTV_walk,
#          1, c="b", alpha=0.5, label="walk")
# plt.legend(loc='upper left')
# plt.xlabel("sec")
# plt.ylabel("Avg TV of derivative amplitude")
# plt.title('Average TV derivative Shocks versus time')
#
# plt.figure()
# plt.scatter(steps_avg_toewalk[:],
#          avg_TV_toewalk,
#          1, c="r", alpha=0.5, label="toewalk")
# plt.scatter(steps_avg_walk[:],
#          avg_TV_walk,
#          1, c="b", alpha=0.5, label="walk")
# plt.legend(loc='upper left')
# plt.xlabel("speed")
# plt.ylabel("Avg TV of  amplitude")
# plt.title('Average TV  Shocks versus speed')
# =============================================================================

plt.show()
