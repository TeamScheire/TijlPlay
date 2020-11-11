#include <Arduino.h>

#define GACCEL 9.80665 // gravitaional acceleration in m/s2 

/*  START USER SETTABLE OPTIONS */
#define SERIALTESTOUTPUT true  // debug info
#define DO_MPU_ACCEL_CALIBRATION false // calibration mpu at start. ONLY WHEN NO MOVEMENT!! ONLY FLAT ON TABLE, Z UPWARD.
#define OUTPUT_ACCEL_2_SERIAL_CSV true // output CSV data with the ACCEL CAPTURE

const int sclPin = D4;
const int sdaPin = D3;
const int txSoftPin = D2;
const int rxSoftPin = D1;

int VOLUME = 8;             //Set volume value. From 0 to 30
const int ProgrammBtn = D6;

#define RAVG_AVG_WIN  8
#define RAVG_AMP_WIN  100 // 33 is around 1 second, so 3 sec avg
const int WALK_AMPLITUDE_MAX = 300;
const int NOTIFY_AFTER_X_SEC_BAD = 1;        // orig: 4
const int UNSET_BAD_AFTER_X_SEC_GOOD = 1;    // orig: 3

//value Cri
const int LEN_CUTOFF1 = 6;
const int CUTOFF_X1[] = {  0, 100, 110, 125, 130, 145};
const int CUTOFF_Y1[] = {200, 365, 375, 460, 610, 811};
//value T
const int LEN_CUTOFF2 = 6;
const int CUTOFF_X2[] = { 0, 60, 118, 145, 155, 165};
const int CUTOFF_Y2[] = {50, 50, 240, 270, 400, 811};
// select which cutoff to use
int use_cutoff = 2;

/*  END USER SETTABLE OPTIONS */

unsigned long timer;

// OLED display with U8g2 lib
#include <U8g2lib.h>   // version 2.16.14

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <TinyMPU6050.h> //adapted version from https://github.com/ingegno/TinyMPU6050

#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"   // from https://github.com/DFRobot/DFRobotDFPlayerMini/archive/1.0.3.zip
// include wifi to allow turning it off
#include "ESP8266WiFi.h" // WiFi Library

bool initialized = false;

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, 
                                            /* clock=SCL*/ sclPin, 
                                            /* data=*/ sdaPin, 
                                            /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather M0 Basic Proto + FeatherWing OLED

//  Constructing MPU-6050
MPU6050 mpu (Wire);

// Battery voltage detection
unsigned int rawADC = 0;
float volt_VBAT = 0.0;
uint16_t encoding_VBAT_glyph = 0x0030;

// Connect chosen RX of Nano to Tx of MP3 player (third pin left)
// Connect chosen TX of Nano to Rx of MP3 player (second pin left)
// Connect 5V line and GND to power MP3 player (as Tx/Rx on that level)
// Connect speaker to left last and third last pin
SoftwareSerial mySoftwareSerial(rxSoftPin, txSoftPin); // RX, TX pins used on Wemos
DFRobotDFPlayerMini myDFPlayer;
int current_track = 1;

#define logo_ingegno_width 32
#define logo_ingegno_height 32
static const unsigned char PROGMEM logo_ingegno_bits[] = {
   0x00, 0x3e, 0x3e, 0x00, 0xc0, 0xbf, 0xfe, 0x00, 0x00, 0xfe, 0x3f, 0x00,
   0x00, 0xff, 0x7f, 0x00, 0x80, 0xff, 0xff, 0x00, 0xc0, 0xff, 0xff, 0x01,
   0xe0, 0xfb, 0xf7, 0x03, 0xe0, 0x71, 0xe3, 0x03, 0xf0, 0x31, 0xe3, 0x07,
   0xf0, 0x3b, 0xf7, 0x07, 0xf0, 0x3f, 0xff, 0x07, 0xf0, 0x7f, 0xff, 0x07,
   0xf0, 0xff, 0xff, 0x07, 0xf0, 0xff, 0xff, 0x07, 0xf0, 0xff, 0xff, 0x07,
   0xe0, 0xff, 0xff, 0x03, 0xe0, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0x01,
   0xe0, 0xff, 0xff, 0x03, 0xf0, 0xff, 0xff, 0x07, 0xf8, 0xc1, 0xff, 0x00,
   0xf8, 0x80, 0x7f, 0x00, 0xfc, 0x18, 0x3f, 0x06, 0x7c, 0x3c, 0x1f, 0x0f,
   0x7e, 0x3c, 0x1e, 0x1f, 0x7e, 0x3c, 0x1e, 0x1f, 0x7e, 0x3c, 0x1e, 0x1f,
   0x7e, 0x3c, 0x1e, 0x1f, 0x7e, 0x3c, 0x1e, 0x3f, 0x7e, 0x3c, 0x1e, 0x3f,
   0x7e, 0x3c, 0x1e, 0x3f, 0x7e, 0x3c, 0x1e, 0x3f };

void printDetail(uint8_t type, int value);

// Some STATUS VARIABLES
#define STATUS_SWITCHED_ON          0
#define STATUS_START_PLAY           1
#define STATUS_PAUSED_PLAY          2
#define STATUS_PAUSED_PLAY_MANUALLY 3
#define STATUS_CONTINUE_PLAY        4

int status;
bool playing = false;
bool statuschanged = false;

// The MODUS VARIABLES
#define MODUS_WALK      0    // only when walking allow to play
#define MODUS_PLAYER    1    // use as a mp3 player

int modus = MODUS_WALK;

// PROGRAMTYPE VARIABLES
#define PROGRAM_TRACK        0
#define PROGRAM_VOLUME       1
#define PROGRAM_CALIBR_CURVE 2

int programtype = PROGRAM_TRACK;

boolean toewalking = false;
float accel;
float window_values[RAVG_AVG_WIN];
float amplitude_window_values[RAVG_AMP_WIN];
float stepspermin_window_values[RAVG_AMP_WIN];
int count = 0;
int count_amplitude = 0;
float accel_avg = 0.;
//peak_resolution_accel = 50
float peaks_sec[3] = {0., 0., 0.};
float peaks[3] = {-1, -1, -1};
int current_peak = 0;
int nr_peaks = 3;
float valleys_sec[3] =  {0., 0., 0.};
float valleys[3] = {2000, 2000, 2000};
int current_valley = 0;
int nr_valleys = 3;
bool peak_found = false;
bool valley_found = false;
float amplitude_avg = 0;
float stepspermin;
float stepspermin_avg = 0;
float cutoff_amp;

float prev_accel;
float prevprev_accel;

bool walking_good = true;
float twalk_bad = 0;
float twalk_badlast = 0;
unsigned long time_walking_good = 0;
bool notify_bad_walk = false;
bool standing_still = false;


boolean ProgrammBtn_PRESSED = LOW;

long ProgrammBtnbuttonTimer = 0;
#define ProgrammBtnminShortPressTime 80
#define ProgrammBtnlongPressTime 750
boolean ProgrammBtnbuttonActive = false;
boolean ProgrammBtnlongPressActive = false;
#define ProgrammBtnNOPRESS    0
#define ProgrammBtnSHORTPRESS 1
#define ProgrammBtnLONGPRESS  2
int ProgrammBtnPressType = ProgrammBtnNOPRESS;


void handleProgrammBtnPress() {
  ProgrammBtnPressType = ProgrammBtnNOPRESS;
      if (digitalRead(ProgrammBtn) == ProgrammBtn_PRESSED) {
        if (ProgrammBtnbuttonActive == false) {
          ProgrammBtnbuttonActive = true;
          ProgrammBtnbuttonTimer = millis();
        }
        if ((millis() - ProgrammBtnbuttonTimer > ProgrammBtnlongPressTime) && (ProgrammBtnlongPressActive == false)) {
          ProgrammBtnlongPressActive = true;
          ProgrammBtnPressType = ProgrammBtnLONGPRESS;
        }
      } else {
        if (ProgrammBtnbuttonActive == true) {
          if (ProgrammBtnlongPressActive == true) {
            ProgrammBtnlongPressActive = false;
          } else {
            //avoid fast fluctuations to be identified as a click
            if (millis() - ProgrammBtnbuttonTimer > ProgrammBtnminShortPressTime)
              ProgrammBtnPressType = ProgrammBtnSHORTPRESS;
          }
          ProgrammBtnbuttonActive = false;
        }
      }
}

void determineStatus() {
  handleProgrammBtnPress();

  if (ProgrammBtnPressType == ProgrammBtnSHORTPRESS) {
    //START STATEMENTS SHORT PRESS
    /* Change state from startup to play and back */
    //Serial.print("Button press ");Serial.print(STATUS_SWITCHED_ON);
    //Serial.print(" " ); Serial.println(standing_still);
    if (status == STATUS_SWITCHED_ON) {
      status = STATUS_START_PLAY;
      statuschanged = true;
    } else if (status == STATUS_START_PLAY || status == STATUS_CONTINUE_PLAY) {
      status = STATUS_PAUSED_PLAY_MANUALLY;
      statuschanged = true;
    } else if (status == STATUS_PAUSED_PLAY_MANUALLY) {
      status = STATUS_CONTINUE_PLAY;
      statuschanged = true;
    } else if (status == STATUS_PAUSED_PLAY && standing_still) {
      // CLICK on paused means program mode IF standing still
      if (programtype == PROGRAM_TRACK) {
        myDFPlayer.stop();
        myDFPlayer.next();
        delay(500);
        current_track = myDFPlayer.readCurrentFileNumber();
        myDFPlayer.loop(current_track);
        delay(100);
        myDFPlayer.pause();
      } else if (programtype == PROGRAM_VOLUME) {
        //reduce the volume with 1, if lowest, restart from 20
        VOLUME = VOLUME - 1;
        if (VOLUME == 0) {VOLUME = 20;}
        myDFPlayer.volume(VOLUME);
      } else if (programtype == PROGRAM_CALIBR_CURVE) {
        if (use_cutoff == 1) {
          use_cutoff = 2;
        } else {
          use_cutoff = 1;
        }
      }

    }
    //END  STATEMENTS SHORT PRESS
  } else if (ProgrammBtnPressType == ProgrammBtnLONGPRESS) {
    //START STATEMENTS LONG PRESS
    if (status == STATUS_PAUSED_PLAY && standing_still) {
      if (programtype == PROGRAM_TRACK) {
        programtype = PROGRAM_VOLUME;
      } else if (programtype == PROGRAM_VOLUME) {
        programtype = PROGRAM_CALIBR_CURVE;
      } else if (programtype == PROGRAM_CALIBR_CURVE) {
        programtype = PROGRAM_TRACK;
      }
    } else {
      //reduce the volume with 1, if lowest, restart from 20
      VOLUME = VOLUME - 1;
      if (VOLUME == 0) {VOLUME = 20;}
      myDFPlayer.volume(VOLUME);
    }
    //END  STATEMENTS LONG PRESS
  } else if (!ProgrammBtnlongPressActive && digitalRead(ProgrammBtn) == ProgrammBtn_PRESSED) {
    //START STATEMENTS PRESS
    //END  STATEMENTS PRESS
  }
}

void meassure_VBAT() {
  rawADC = analogRead(A0);
  volt_VBAT = rawADC / 1023.0 * 4.2;
  // use u8g2_font_battery19_tm a width 8 height 19 font.
  // for Wemos 3.5 - 3.65 = 1 bar 0x0031
  //           3.65- 3.8  = 2 bar 0x0032 
  //           3.8 - 3.95 = half  0x0033
  //           3.95 - 4.1 = 4 bar 0x0034
  //           > 4.1      = full  0x0035 
  if (volt_VBAT < 3.5) {
    encoding_VBAT_glyph = 0x0030;
  } else if (volt_VBAT < 3.65) {
    encoding_VBAT_glyph = 0x0031;
  } else if (volt_VBAT < 3.8) {
    encoding_VBAT_glyph = 0x0032;
  } else if (volt_VBAT < 3.95) {
    encoding_VBAT_glyph = 0x0033;
  } else if (volt_VBAT < 4.1) {
    encoding_VBAT_glyph = 0x0034;
  } else {
    encoding_VBAT_glyph = 0x0035;
  }
}

void show_screen() {
    u8g2.clearBuffer(); // clear the internal memory
    u8g2.drawXBMP(0, 0, logo_ingegno_width, 
                         logo_ingegno_height, 
                         logo_ingegno_bits);
    u8g2.setFont(u8g2_font_unifont_t_symbols);  // a 16x16 font
    if (status == STATUS_START_PLAY || status == STATUS_CONTINUE_PLAY) {
      u8g2.drawGlyph(104,32,0x23f5);  
    } else if (status == STATUS_PAUSED_PLAY_MANUALLY) {
      u8g2.drawGlyph(104,32,0x23f8);
    } else if (status == STATUS_PAUSED_PLAY) {
      u8g2.drawGlyph(104,32,0x23f3);
    } else {
      //switched on
      u8g2.drawGlyph(104,32,0x23f9);
    }

    if (notify_bad_walk) {
      // show error
      if (standing_still) {
        u8g2.drawGlyph(88,32,0x2615);
      } else {
        // toewalking
        u8g2.drawGlyph(88,32,0x2716);
      }
    } else {
      // show OK
      u8g2.drawGlyph(88,32,0x2714);
    }

    u8g2.setFont(u8g2_font_6x12_tf);  // a 6x12 font
    // show current Track
    u8g2.setCursor(104,12);
    if (programtype == PROGRAM_TRACK) {
      u8g2.print("t");
    } else {
      u8g2.print("T");
    }
    u8g2.setCursor(110,12);
    u8g2.print(current_track);
    // show volume
    u8g2.setCursor(86,12);
    if (programtype == PROGRAM_VOLUME && standing_still) {
      u8g2.print("v");
    } else {
      u8g2.print("V");
    }
    u8g2.setCursor(92,12);
    u8g2.print(VOLUME);
    // show calibration
    u8g2.setCursor(74,12);
    if (programtype == PROGRAM_CALIBR_CURVE) {
      u8g2.print("c");
    } else {
      u8g2.print("C");
    }
    u8g2.setCursor(80,12);
    u8g2.print(use_cutoff);
    
    // show battery icon
    meassure_VBAT();
    u8g2.setFont(u8g2_font_battery19_tn);
    u8g2.drawGlyph(120,32,encoding_VBAT_glyph);

    // show accel data
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(33,7);
    u8g2.print(mpu.GetAccX());
    u8g2.setCursor(33,15);
    u8g2.print(mpu.GetAccY());
    u8g2.setCursor(33,23);
    u8g2.print(mpu.GetAccZ());
    // computed data
    u8g2.setCursor(33,32);
    u8g2.print(int(amplitude_avg));
    u8g2.setCursor(55,32);
    u8g2.print(int(stepspermin_avg));
    u8g2.sendBuffer();
/*    
  if (SERIALTESTOUTPUT) {
    Serial.print("AccX = ");
    Serial.print(mpu.GetAccX());
    Serial.print(" m/s² - ");
    Serial.print("AccY = ");
    Serial.print(mpu.GetAccY());
    Serial.print(" m/s² - ");
    Serial.print("AccZ = ");
    Serial.print(mpu.GetAccZ());
    Serial.println(" m/s²");
  }
  */
}

int ard_effect0_status = -1;
unsigned long ard_effect0_start, ard_effect0_time;
#define EFFECT0_PERIOD 1000

void update_screen() {
  //Variables of this effect are reffered to with ard_effect0
  boolean restart = false;
  ard_effect0_time = millis() - ard_effect0_start;
  if (ard_effect0_time > EFFECT0_PERIOD) {
    //end effect, make sure it restarts
    if (ard_effect0_status > -1) {
    }
    restart = true;
    ard_effect0_status = -1;
    ard_effect0_start = ard_effect0_start + ard_effect0_time;
    ard_effect0_time = 0;
  }
  if (not restart && ard_effect0_status == -1) {
    ard_effect0_status = 0;
    ard_effect0_start = ard_effect0_start + ard_effect0_time;
    ard_effect0_time = 0;
    
    show_screen();
  }
}

float mymean(float floatarray[], int len_array){
 int i;
 float sum = 0;
 float average = 0.0;
 /* calculate the sum of grades using for loop*/
    for(i = 0; i < len_array; i++){
       sum = sum + floatarray[i];
    }
 average = sum/float(len_array);

 return average;
}
float mymean(int intarray[], int len_array){
 int i;
 float sum = 0;
 float average = 0.0;
 /* calculate the sum of grades using for loop*/
    for(i = 0; i < len_array; i++){
       sum = sum + intarray[i];
    }
 average = sum/float(len_array);

 return average;
}

float determine_cutoff_amp(float stepspermin_avg, const int CUTOFF_X[],
                           const int CUTOFF_Y[], const int LEN_CUTOFF){
    int indpos = 1;

    while (CUTOFF_X[indpos] < stepspermin_avg) {
        indpos += 1;
        if (indpos == LEN_CUTOFF) {
            indpos -= 1;
            break;
        }
    }
    //indpos is now the right point of where stepspermin are
    //interpolate at the steps to find the amplitude cutoff value
    float cutoff_amp = CUTOFF_Y[indpos-1] 
        + float(CUTOFF_Y[indpos]-CUTOFF_Y[indpos-1]) 
            / float(CUTOFF_X[indpos]-CUTOFF_X[indpos-1]) 
            * (stepspermin_avg-CUTOFF_X[indpos-1]);
    return cutoff_amp;
}

void measureAccel(){
    // does the actual acceleration measurement.
    // should happen once every 30 ms via effect perform_accel_meas()
    mpu.Execute();
    //compute the acceleration in milli g. No movement: 1024 milli-g
    accel = 1000 * sqrt(pow(mpu.GetAccX(),2)
                        + pow(mpu.GetAccY(),2)
                        + pow(mpu.GetAccZ(),2));

    // process the data to determine peaks and valleys
    window_values[count] = accel;
    count = count + 1;
    count = count % RAVG_AVG_WIN;
    accel_avg = mymean(window_values, RAVG_AVG_WIN);
    float sec = timer / 1000.0;  //time in sec
    if (accel_avg > peaks[current_peak] && accel_avg > prev_accel) {
        // going towards a peak
        peaks[current_peak] = accel_avg;
        peaks_sec[current_peak] = sec;
        if (accel_avg > prev_accel && accel_avg > prevprev_accel) {
            peak_found = true;
        }
    } else if (peak_found && accel_avg < peaks[current_peak] - 50) {
        // peak finished, go to next peak
        current_peak = current_peak + 1;
        current_peak = current_peak % nr_peaks;
        peaks[current_peak] = 0;
        peaks_sec[current_peak] = sec;
        peak_found = false;
    } else if (sec - peaks_sec[current_peak] > 0.8) {
        // force stop of peak
        current_peak = current_peak + 1;
        current_peak = current_peak % nr_peaks;
        peaks[current_peak] = 0;
        peaks_sec[current_peak] = sec;
        peak_found = false;
    }

    if (accel_avg < valleys[current_valley] && accel_avg < prev_accel) {
        // going towards a valley
        valleys[current_valley] = accel_avg;
        valleys_sec[current_valley] = sec;
        if (accel_avg < prev_accel && accel_avg < prevprev_accel) {
            valley_found = true;
        }
    } else if (valley_found && accel_avg > valleys[current_valley] + 50) {
        // valley finished, go to next valley
        current_valley += 1;
        current_valley = current_valley % nr_valleys;
        valleys[current_valley] = 3000;
        valleys_sec[current_valley] = sec;
        valley_found = false;
    } else if (sec - valleys_sec[current_valley] > 0.8) {
        //force stop of valley
        current_valley += 1;
        current_valley = current_valley % nr_valleys;
        valleys[current_valley] = 3000;
        valleys_sec[current_valley] = sec;
        valley_found = false;
    }

    prevprev_accel = prev_accel;
    prev_accel = accel_avg;

    int tmppeakprev = current_peak - 1;
    if (tmppeakprev < 0) {tmppeakprev = nr_peaks - 1;}
    int tmppeakprevprev = tmppeakprev - 1;
    if (tmppeakprevprev < 0) {tmppeakprevprev = nr_peaks - 1;}
    int tmpvalleyprev = current_valley - 1;
    if (tmpvalleyprev < 0) {tmpvalleyprev = nr_valleys - 1;}
    int tmpvalleyprevprev = tmpvalleyprev - 1;
    if (tmpvalleyprevprev < 0) {tmpvalleyprevprev = nr_valleys - 1;}
        
    if (amplitude_avg > 120) {
        stepspermin = 60/(peaks_sec[tmppeakprev]-peaks_sec[tmppeakprevprev]);
    } else {
        stepspermin = 60;  // avoid in average steps dropping too fast, Normal walk is > 80....
    }
    // amplitude average data
    if (peaks[tmppeakprev] != -1 && valleys[tmpvalleyprev] != 2000) {
        amplitude_window_values[count_amplitude] = peaks[tmppeakprev] - valleys[tmpvalleyprev];
        stepspermin_window_values[count_amplitude] = stepspermin;
        count_amplitude = count_amplitude + 1;
        count_amplitude = count_amplitude % RAVG_AMP_WIN;
    }
    amplitude_avg = int(round(mymean(amplitude_window_values, RAVG_AMP_WIN)));
    stepspermin_avg = int(round(mymean(stepspermin_window_values, RAVG_AMP_WIN)));

    // with the new acces data added, determine if toewalking or not
    determine_toewalk(amplitude_avg, stepspermin_avg);

    // if serial output wanted, write out the measurement
    if (OUTPUT_ACCEL_2_SERIAL_CSV && SERIALTESTOUTPUT) {
      Serial.print(sec);Serial.print(",");
      Serial.print(accel);Serial.print(",");
      Serial.print(current_peak);Serial.print(",");
      Serial.print(stepspermin);Serial.print(",");
      Serial.print(peaks[tmppeakprev]);Serial.print(",");
      Serial.print(peaks_sec[tmppeakprev]);Serial.print(",");
      Serial.print(valleys[tmpvalleyprev]);Serial.print(",");
      Serial.print(valleys_sec[tmpvalleyprev]);Serial.print(",");
      Serial.print(amplitude_avg);Serial.print(",");
      Serial.print(accel_avg);
      //Serial.println(",");
    }
            
}

int ard_effect1_status = -1;
unsigned long ard_effect1_start, ard_effect1_time;
#define EFFECT1_PERIOD 30

void perform_accel_meas() {
  //Variables of this effect are reffered to with ard_effect1
  boolean restart = false;
  ard_effect1_time = millis() - ard_effect1_start;
  if (ard_effect1_time > EFFECT1_PERIOD) {
    //end effect, make sure it restarts
    if (ard_effect1_status > -1) {
    }
    restart = true;
    ard_effect1_status = -1;
    ard_effect1_start = ard_effect1_start + ard_effect1_time;
    ard_effect1_time = 0;
  }
  if (not restart && ard_effect1_status == -1) {
    ard_effect1_status = 0;
    ard_effect1_start = ard_effect1_start + ard_effect1_time;
    ard_effect1_time = 0;
  measureAccel();
  }
}

// initialize display 
void setup_display() {
  if (not initialized)
  {    
    u8g2.clearBuffer(); // clear the internal memory
    u8g2.setFont(u8g2_font_smart_patrol_nbp_tf); // choose a suitable font

    u8g2.drawXBMP(0, 0, logo_ingegno_width, 
                         logo_ingegno_height, 
                         logo_ingegno_bits);
    u8g2.sendBuffer();
    u8g2.drawStr(68,10,"TijlPlay");  // write something to the internal memory
    u8g2.drawStr(100,32,"OFF");  // write something to the internal memory
    
    u8g2.sendBuffer();          // transfer internal memory to the display

    if (SERIALTESTOUTPUT) {
      Serial.println("Display init done");
    }
    delay(1000);  
  }
}

void setup_mpu(){
  mpu.Initialize(sdaPin, sclPin);
  
  if (DO_MPU_ACCEL_CALIBRATION) {
    //increase z deadzone from default 0.002 m/s2
    mpu.SetAccelDeadzone(0.02);
    mpu.SetZAccelDeadzone(0.02);
    mpu.SetGyroDeadzone(0.9);
    mpu.SetZGyroDeadzone(0.9);
    if (SERIALTESTOUTPUT) {
      Serial.println("=====================================");
      Serial.println("Starting calibration...");
    }
    mpu.Calibrate();
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(33,16);
    u8g2.print(mpu.GetAccXOffset());
    u8g2.setCursor(33,24);
    u8g2.print(mpu.GetAccYOffset());
    u8g2.setCursor(33,32);
    u8g2.print(mpu.GetAccZOffset());
    u8g2.sendBuffer();
    if (SERIALTESTOUTPUT) {
      Serial.println("Calibration complete!");
      Serial.println("Offsets:");
      Serial.print("AccX Offset = ");
      Serial.println(mpu.GetAccXOffset());
      Serial.print("AccY Offset = ");
      Serial.println(mpu.GetAccYOffset());
      Serial.print("AccZ Offset = ");
      Serial.println(mpu.GetAccZOffset());
      Serial.print("GyroX Offset = ");
      Serial.println(mpu.GetGyroXOffset());
      Serial.print("GyroY Offset = ");
      Serial.println(mpu.GetGyroYOffset());
      Serial.print("GyroZ Offset = ");
      Serial.println(mpu.GetGyroZOffset());
      Serial.println("=====================");
      Serial.println("Current Values");
      mpu.Execute();
      Serial.print("AccX = ");
      Serial.print(mpu.GetAccX());
      Serial.print(" m/s² - ");
      Serial.print("AccY = ");
      Serial.print(mpu.GetAccY());
      Serial.print(" m/s² - ");
      Serial.print("AccZ = ");
      Serial.print(mpu.GetAccZ());
      Serial.println(" m/s²");
      Serial.println("=====================");
    }
  }
}

void setup_mp3() {
  mySoftwareSerial.begin(9600);
  delay(1000);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    if (SERIALTESTOUTPUT) {
      Serial.println(F("Unable to begin:"));
      Serial.println(F("1.Please recheck the connection!"));
      Serial.println(F("2.Please insert the SD card!"));
    }
    while(true);
  }
  
  myDFPlayer.volume(VOLUME);  //Set volume value. From 0 to 30

  if (SERIALTESTOUTPUT) {
    Serial.println(F("MP3Player Mini online."));
  }
}

void setup_walkdata() {
  for (int i = 0; i < RAVG_AVG_WIN; i++) {
    window_values[i] = 0.;
  }
  for (int i = 0; i < RAVG_AMP_WIN; i++) {
    amplitude_window_values[i] = 0.;
    stepspermin_window_values[i] = 0.;
  }
  
  measureAccel();
  prev_accel = accel;
  prevprev_accel = accel;
}

void setup() {
  if (SERIALTESTOUTPUT) Serial.begin(115200);
  
  pinMode(ProgrammBtn, INPUT_PULLUP);
  // battery charge on A0 using 100k between Vbat and ADC
  // Voltage divider of 100k+220k over 100k gives 100/420k 
  // ergo 4.2V -> 1Volt,  Max input on A0 of ESP = 1Volt -> 1023
  // Actual battery voltage hence 4.2*(Raw/1023)=Vbat
  pinMode(A0, INPUT);  


  Wire.begin(sdaPin, sclPin);
  u8g2.begin();  // uses wire
  setup_display();
  
  //switch off wifi https://arduino-esp8266.readthedocs.io/en/2.5.2/esp8266wifi/generic-class.html#mode
  WiFi.mode(WIFI_OFF);
  delay(2000);
  
  ard_effect0_status = -1;
  ard_effect0_start = millis();
  ard_effect1_status = -1;
  ard_effect1_start = millis();

  setup_mpu();

  setup_mp3();
  
  setup_walkdata();

  status = STATUS_SWITCHED_ON;
  playing = false;
  
  if (modus == MODUS_WALK) {
      status = STATUS_PAUSED_PLAY;
      playing = false;
  }
  if (SERIALTESTOUTPUT) {
    Serial.println("Setup Finished");
  }
}

void on_in_toewalk_region() {
    toewalking = true;
    float sec = timer / 1000.0;
    twalk_badlast = sec;
    // walking bad, allow for some sec
    if (walking_good) {
        // we were walking good, now bad
        walking_good = false;
        twalk_bad = sec;
    } else if (sec - twalk_bad > NOTIFY_AFTER_X_SEC_BAD) {
        // notify after 4 sec of walking bad
        notify_bad_walk = true;
    }
}


void on_in_walk_region(float stepspermin_avg) {
    float sec = timer / 1000.;
    toewalking = false;
    if (stepspermin_avg <= 80) {
        // not walking
        standing_still = true;
        twalk_badlast = sec;
        if (walking_good) {
            // we were walking good, now bad as not fast enough
            walking_good = false;
            twalk_bad = sec;
        } else if (sec - twalk_bad > NOTIFY_AFTER_X_SEC_BAD) {
            // notify after X sec of standing still
            notify_bad_walk = true;
        }
    } else {
        // not in toewalk and not standing still. All is good!
        // we might have been walking bad, unset if long enough walking good
        standing_still = false;
        programtype = PROGRAM_VOLUME;
        if (!walking_good) {
            //unset walking bad if last walking bad was X sec ago
            if (sec - twalk_badlast > UNSET_BAD_AFTER_X_SEC_GOOD) {
                walking_good = true;
                notify_bad_walk = false;
            }
        }
    }
}

/* 
 * algorithm to determine if doing toewalk 
 * returns true if toewalking
 */
void determine_toewalk(float amplitude_avg, float stepspermin_avg) {
    // we work with alogithm based on cutoff line
    if (use_cutoff == 2) {
        cutoff_amp = determine_cutoff_amp(stepspermin_avg, CUTOFF_X2, CUTOFF_Y2, LEN_CUTOFF2);
    } else {
        cutoff_amp = determine_cutoff_amp(stepspermin_avg, CUTOFF_X1, CUTOFF_Y1, LEN_CUTOFF1);
    }

    if (stepspermin_avg > 80 && amplitude_avg > cutoff_amp) {
        standing_still = false;
        programtype = PROGRAM_VOLUME;
        on_in_toewalk_region();
    } else {
        on_in_walk_region(stepspermin_avg);
    }
        
    //toewalking = false;
}

void loop() {  
  timer = millis();
  
  determineStatus();
  perform_accel_meas();
  //determine_toewalk();

  
  
  if (modus == MODUS_WALK) {
    if (statuschanged) {
      // Only possible when not paused. MP3 is playing. Should stop if toewalk
      if (status == STATUS_SWITCHED_ON) {
        myDFPlayer.stop();
        playing = false;
      } else if (status == STATUS_START_PLAY) {
        myDFPlayer.loop(current_track);   //loop the first mp3
        playing = true;
        //myDFPlayer.play(current_track);   //Play the first mp3
      } else if (status == STATUS_CONTINUE_PLAY) {
        myDFPlayer.start();   //loop the first mp3
        playing = true;
      } else if (status == STATUS_PAUSED_PLAY_MANUALLY) {
        myDFPlayer.pause();   //loop the first mp3
        playing = false;
      }
      statuschanged = false;
    } else {
      // check if playing
      if (playing && notify_bad_walk) {
        //pause play, toewalking or standing still
        myDFPlayer.pause();
        status = STATUS_PAUSED_PLAY;
        playing = false;
      } else if (status == STATUS_PAUSED_PLAY && !notify_bad_walk) {
        //play was paused but now walking good, start again
        myDFPlayer.start();
        status = STATUS_CONTINUE_PLAY;
        playing = true;
      }
    }
  } else {
    // modus == MODUS_PLAYER
    if (status == STATUS_PAUSED_PLAY) {
      // this status is not possible in MODUS_PLAYER, Change.
      status = STATUS_PAUSED_PLAY_MANUALLY;
      statuschanged = true;
    }
    if (statuschanged) {
      // Only possible when not paused. MP3 is playing. Should stop if toewalk
      if (SERIALTESTOUTPUT) {
        Serial.print("Status Changed to ");
        Serial.println(status);
      }
      if (status == STATUS_SWITCHED_ON) {
        myDFPlayer.stop();
        playing = false;
      } else if (status == STATUS_START_PLAY) {
        myDFPlayer.loop(1);   //loop the first mp3
        playing = true;
        //myDFPlayer.play(1);   //Play the first mp3
      } else if (status == STATUS_CONTINUE_PLAY) {
        myDFPlayer.start();   //loop the first mp3
        playing = true;
      } else if (status == STATUS_PAUSED_PLAY_MANUALLY) {
        myDFPlayer.pause();   //loop the first mp3
        playing = false;
      }
      statuschanged = false;
    }
  }

  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }

  // every xx ms we update the screen
  update_screen();
  
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
