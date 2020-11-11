### SETTINGS START ###
RAVG_AVG_WIN = 8
RAVG_AMP_WIN = 100 # 50 is around 1 second
DELAY_MEASUREMENT = 1
WALK_AMPLITUDE_MAX = 300
NOTIFY_AFTER_X_SEC_BAD = 1        # orig: 4
UNSET_BAD_AFTER_X_SEC_GOOD = 1    # orig: 3

#value Cri
CUTOFF_X1 = [  0, 100, 110, 125, 130, 145]
CUTOFF_Y1 = [200, 365, 375, 460, 610, 811]
#value T
CUTOFF_X2 = [ 0, 60, 118, 145, 155, 165]
CUTOFF_Y2 = [50, 50, 240, 270, 400, 811]


#CUTOFF_X = [ 0, 60, 118, 145, 155, 165]
#CUTOFF_Y = [50, 50, 240, 270, 400, 811]


LEN_CUTOFF1 = len(CUTOFF_X1)
LEN_CUTOFF2 = len(CUTOFF_X2)

use_cutoff = 2

### SETTINGS END ###

from microbit import accelerometer, button_b, pin1, pin2, display, Image, sleep
import math
import utime
#import music

# variables
current_sec = float(utime.ticks_ms())/1000.  # some reference time in ms
# read in currect accel data
accel = math.sqrt(float(accelerometer.get_x())**2
                        + float(accelerometer.get_y())**2
                        + float(accelerometer.get_z())**2)

window_values = [0] * RAVG_AVG_WIN
amplitude_window_values = [0] * RAVG_AMP_WIN
stepspermin_window_values = [0] * RAVG_AMP_WIN
count = 0
count_amplitude = 0
accel_avg = 0.
#peak_resolution_accel = 50
peaks_sec = [current_sec, current_sec, current_sec]
peaks = [-1, -1, -1]
current_peak = 0
nr_peaks = 3
valleys_sec = [current_sec, current_sec, current_sec]
valleys = [2000, 2000, 2000]
current_valley = 0
nr_valleys = 3
peak_found = False
valley_found = False
amplitude_avg = 0
stepspermin_avg = 0

prev_accel = accel
prevprev_accel = accel

walking_good = True
twalk_bad = -1
twalk_badlast = -1
time_walking_good = current_sec
notify_bad_walk = False
standing_still = False

# functions
def mymean(mylist):
    return sum(mylist) / float(len(mylist))

def determine_cutoff_amp(stepspermin_avg, CUTOFF_X, CUTOFF_Y, LEN_CUTOFF):
    indpos = 1

    while CUTOFF_X[indpos] < stepspermin_avg:
        indpos += 1
        if indpos == LEN_CUTOFF:
            indpos -= 1
            break
    #indpos is now the right point of where stepspermin are
    #interpolate at the steps to find the amplitude cutoff value
    cutoff_amp = CUTOFF_Y[indpos-1] \
        + (CUTOFF_Y[indpos]-CUTOFF_Y[indpos-1]) \
            / (CUTOFF_X[indpos]-CUTOFF_X[indpos-1]) \
            * (stepspermin_avg-CUTOFF_X[indpos-1])
    return cutoff_amp

def determine_toewalk(amplitude_avg, stepspermin_avg):
    def on_in_toewalk_region():
        global walking_good, twalk_bad, twalk_badlast, notify_bad_walk
        display.show(Image.CHESSBOARD)
        twalk_badlast = sec
        # walking bad, allow for some sec
        if walking_good:
            # we were walking good, now bad
            walking_good = False
            twalk_bad = sec
        elif sec - twalk_bad > NOTIFY_AFTER_X_SEC_BAD:
            # notify after 4 sec of walking bad
            notify_bad_walk = True

    def on_in_walk_region(stepspermin_avg):
        global walking_good, twalk_bad, twalk_badlast, \
                notify_bad_walk, standing_still

        if stepspermin_avg <= 80:
            # not walking
            standing_still = True
            display.show(Image.ARROW_N)
            twalk_badlast = sec
            if walking_good:
                # we were walking good, now bad as not fast enough
                walking_good = False
                twalk_bad = sec
            elif sec - twalk_bad > NOTIFY_AFTER_X_SEC_BAD:
                # notify after X sec of standing still
                notify_bad_walk = True
        else:
            # not in toewalk and not standing still. All is good!
            display.show(Image.SQUARE_SMALL)

            # we might have been walking bad, unset if long enough walking good
            if not walking_good:
                #unset walking bad if last walking bad was X sec ago
                if sec - twalk_badlast > UNSET_BAD_AFTER_X_SEC_GOOD:
                    walking_good = True
                    notify_bad_walk = False

    # we work with alogithm based on cutoff line
    if use_cutoff == 2:
        cutoff_amp = determine_cutoff_amp(stepspermin_avg, CUTOFF_X2, CUTOFF_Y2, LEN_CUTOFF2)
    else:
        cutoff_amp = determine_cutoff_amp(stepspermin_avg, CUTOFF_X1, CUTOFF_Y1, LEN_CUTOFF1)

    if stepspermin_avg > 80 and amplitude_avg > cutoff_amp:
        standing_still = False
        on_in_toewalk_region()
    else:
        on_in_walk_region(stepspermin_avg)

# main program
while True:
    if button_b.was_pressed():
        use_cutoff += 1
        if use_cutoff > 2:
            use_cutoff = 1
    sec = float(utime.ticks_ms())/1000.  # some reference time in ms
    # read in currect accel data
    accel = math.sqrt(float(accelerometer.get_x())**2
                            + float(accelerometer.get_y())**2
                            + float(accelerometer.get_z())**2)

    # process the data to determine peaks and valleys
    window_values[count] = accel
    count = count + 1
    count = count % RAVG_AVG_WIN
    accel_avg = mymean(window_values)
    if accel_avg > peaks[current_peak] and accel_avg > prev_accel:
        # going towards a peak
        peaks[current_peak] = accel_avg
        peaks_sec[current_peak] = sec
        if accel_avg > prev_accel and accel_avg > prevprev_accel:
            peak_found = True
    elif peak_found and accel_avg < peaks[current_peak] - 50:
        # peak finished, go to next peak
        current_peak = current_peak + 1
        current_peak = current_peak % nr_peaks
        peaks[current_peak] = 0
        peaks_sec[current_peak] = sec
        peak_found = False
    elif sec - peaks_sec[current_peak] > 0.8:
        # force stop of peak
        current_peak = current_peak + 1
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
    elif valley_found and accel_avg > valleys[current_valley] + 50:
        # valley finished, go to next valley
        current_valley += 1
        current_valley = current_valley % nr_valleys
        valleys[current_valley] = 3000
        valleys_sec[current_valley] = sec
        valley_found = False
    elif sec - valleys_sec[current_valley] > 0.8:
        #force stop of valley
        current_valley += 1
        current_valley = current_valley % nr_valleys
        valleys[current_valley] = 3000
        valleys_sec[current_valley] = sec
        valley_found = False

    prevprev_accel = prev_accel
    prev_accel = accel_avg

    if amplitude_avg > 120:
        stepspermin = 60/(peaks_sec[current_peak-1]-peaks_sec[current_peak-2])
    else:
        stepspermin = 60  # avoid in average steps dropping too fast, Normal walk is > 80....
    # amplitude average data
    if peaks[current_peak-1] != -1 and valleys[current_valley-1] != 2000:
        amplitude_window_values[count_amplitude] = peaks[current_peak-1] - valleys[current_valley-1]
        stepspermin_window_values[count_amplitude] = stepspermin
        count_amplitude = count_amplitude + 1
        count_amplitude = count_amplitude % RAVG_AMP_WIN
    amplitude_avg = int(round(mymean(amplitude_window_values), 0))
    stepspermin_avg = int(round(mymean(stepspermin_window_values), 0))

    determine_toewalk(amplitude_avg, stepspermin_avg)

    if notify_bad_walk:
        # set pin 1 high
        pin1.write_digital(1)
    else:
#        pin0.write_digital(0)
        pin1.write_digital(0)

    if standing_still:
        pin2.write_digital(1)
    else:
        pin2.write_digital(0)

    #debug prints
    print ((sec, accel, current_peak, stepspermin,
            peaks[current_peak-1], peaks_sec[current_peak-1],
            valleys[current_valley-1], valleys_sec[current_valley-1],
            amplitude_avg, accel_avg))

    sleep(DELAY_MEASUREMENT)