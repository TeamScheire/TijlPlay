/***************
 * Ingegno ToeWalkBelt for Team Scheire Big Fix  
 * 
 * Only play music when normal walking is detected
 * 
 * Combination of Nano and Microbit
 * 
 */

/***************************************************
 MP3 player code Based on: DFPlayer - A Mini MP3 Player For Arduino
 ***************************************************
 Created 2016-12-07  By [Angelo qiao](Angelo.qiao@dfrobot.com)

 GNU Lesser General Public License.
 See <http://www.gnu.org/licenses/> for details.
 All above must be included in any redistribution
 ****************************************************/

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"

/* CONFIGURATION START */
int VOLUME = 8;                     //Set volume value. From 0 to 30
const int toewalkPin = A1;          //Analog pin used on Nano to read toewalk value
const int standingStillPin = A2;    //Analog pin used on Nano to read standing still value MicroBit
const int ProgrammBtn = 2;
const int ModusSwitch = 3;
const bool ModusSwitchPresent = false;
#define SERIALTESTOUTPUT false  // debug info

#define HAS_STANDINGSTILL_PIN false

/* CONFIGURATION END   */

const int sclPin = A5;
const int sdaPin = A4;

// Connect chosen RX of Nano to Tx of MP3 player (third pin left)
// Connect chosen TX of Nano to Rx of MP3 player (second pin left)
// Connect 5V line and GND to power MP3 player (as Tx/Rx on that level)
// Connect speaker to left last and third last pin
SoftwareSerial mySoftwareSerial(10, 11); // RX, TX pins used on Nano
DFRobotDFPlayerMini myDFPlayer;

// OLED display with U8g2 lib
#include <U8g2lib.h>   // version 2.16.14

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, 
                                            /* clock=SCL*/ SCL, 
                                            /* data=*/ SDA, 
                                            /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather M0 Basic Proto + FeatherWing OLED

int current_track = 1;

int toewalkval = 0;
int standingval = 0;
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

void determineModus() {
  if (digitalRead(ModusSwitch) == HIGH) {
    modus = MODUS_WALK;
  } else {
    modus = MODUS_PLAYER;
  }
}

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
    if (status == STATUS_SWITCHED_ON) {
      status = STATUS_START_PLAY;
      statuschanged = true;
    } else if (status == STATUS_START_PLAY || status == STATUS_CONTINUE_PLAY) {
      status = STATUS_PAUSED_PLAY_MANUALLY;
      statuschanged = true;
    } else if (status == STATUS_PAUSED_PLAY_MANUALLY) {
      status = STATUS_CONTINUE_PLAY;
      statuschanged = true;
    } else if (status == STATUS_PAUSED_PLAY) {
      // CLICK on paused means program mode
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
      }
    //END  STATEMENTS SHORT PRESS
    }
  } else if (ProgrammBtnPressType == ProgrammBtnLONGPRESS) {
    //START STATEMENTS LONG PRESS
    if (status == STATUS_PAUSED_PLAY) {
      if (programtype == PROGRAM_TRACK) {
        programtype = PROGRAM_VOLUME;
      } else if (programtype == PROGRAM_VOLUME) {
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

// initialize display 
void setup_display() {
  u8g2.clearBuffer(); // clear the internal memory
  u8g2.setFont(u8g2_font_smart_patrol_nbp_tf); // choose a suitable font

//    u8g2.drawXBMP(0, 0, logo_ingegno_width, 
//                         logo_ingegno_height, 
//                         logo_ingegno_bits);
  u8g2.sendBuffer();
  u8g2.drawStr(68,10,"TijlPlay");  // write something to the internal memory
  u8g2.drawStr(100,32,"OFF");  // write something to the internal memory
  
  u8g2.sendBuffer();          // transfer internal memory to the display

  if (SERIALTESTOUTPUT) {
    Serial.println("Display init done");
  }
  delay(1000);
}

void show_screen() {
    u8g2.clearBuffer(); // clear the internal memory
//    u8g2.drawXBMP(0, 0, logo_ingegno_width, 
//                         logo_ingegno_height, 
//                         logo_ingegno_bits);
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

    if (toewalkval > 500) {
      // show error
        // toewalking or standing still
        u8g2.drawGlyph(88,32,0x2716);
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
    if (programtype == PROGRAM_VOLUME) {
      u8g2.print("v");
    } else {
      u8g2.print("V");
    }
    u8g2.setCursor(92,12);
    u8g2.print(VOLUME);
    // show calibration
//    u8g2.setCursor(74,12);
//    if (programtype == PROGRAM_CALIBR_CURVE) {
//      u8g2.print("c");
//    } else {
//      u8g2.print("C");
//    }
//    u8g2.setCursor(80,12);
//    u8g2.print(use_cutoff);
    
//    // show battery icon
//    meassure_VBAT();
//    u8g2.setFont(u8g2_font_battery19_tn);
//    u8g2.drawGlyph(120,32,encoding_VBAT_glyph);
//
//    // show accel data
//    u8g2.setFont(u8g2_font_5x7_tr);
//    u8g2.setCursor(33,7);
//    u8g2.print(mpu.GetAccX());
//    u8g2.setCursor(33,15);
//    u8g2.print(mpu.GetAccY());
//    u8g2.setCursor(33,23);
//    u8g2.print(mpu.GetAccZ());
//    // computed data
//    u8g2.setCursor(33,32);
//    u8g2.print(int(amplitude_avg));
//    u8g2.setCursor(55,32);
//    u8g2.print(int(stepspermin_avg));
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

void setup()
{
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  
  pinMode(ProgrammBtn, INPUT_PULLUP);
  if (ModusSwitchPresent) pinMode(ModusSwitch, INPUT_PULLUP);

  //Wire.begin();
  u8g2.begin();  // uses wire
  
  setup_display();
  
  ard_effect0_status = -1;
  ard_effect0_start = millis();
  
  Serial.println();
  Serial.println(F("Team Scheire ToeWalkBelt"));
  Serial.println(F("Initializing MP3Player ... (May take 3~5 seconds)"));

  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while(true);
  }
  Serial.println(F("MP3Player Mini online."));


  status = STATUS_SWITCHED_ON;
  playing = false;
  if (ModusSwitchPresent) {
    determineModus();
  } else {
    modus = MODUS_WALK;
  }
  if (modus == MODUS_WALK) {
    //if (HAS_STANDINGSTILL_PIN) {
      // in modus walk with standing still detection: play when walking,
      //   no extra button press needed
      status = STATUS_PAUSED_PLAY;
      playing = false;
    //}
  }

  myDFPlayer.volume(VOLUME);  //Set volume value. From 0 to 30
}

void loop()
{
  static unsigned long timer = millis();

  if (ModusSwitchPresent) {
    determineModus();
  } else {
    modus = MODUS_WALK;
  }
  determineStatus();
  toewalkval = analogRead(toewalkPin);  // read the input pin
  if (HAS_STANDINGSTILL_PIN) {
    standingval = analogRead(standingStillPin);
  } else {
    standingval = 0;
  }

  if (modus == MODUS_WALK) {
    if (statuschanged) {
      // Only possible when not paused. MP3 is playing. Should stop if toewalk
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
    } else {
      // check if playing
      if (playing && toewalkval > 500) {
        //pause play, toewalking
        myDFPlayer.pause();
        status = STATUS_PAUSED_PLAY;
        playing = false;
      } else if (status == STATUS_PAUSED_PLAY && toewalkval <500) {
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

  // every xx ms we update the screen
  update_screen();
  
  if (myDFPlayer.available()) {
    printDetail(myDFPlayer.readType(), myDFPlayer.read()); //Print the detail message from DFPlayer to handle different errors and states.
  }
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
