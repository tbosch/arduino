/**********************************************************************
  Product     : Freenove 4WD Car for UNO
  Description : Control ws2812b led through freenove_controller.
  Auther      : www.freenove.com
  Modification: 2019/08/03
**********************************************************************/
#include "Freenove_WS2812B_RGBLED_Controller.h"
#include "SCMD.h"
#include "SCMD_config.h" //Contains #defines for common SCMD register names and values

#define DEBUG 1
// TODO: Disable the motors when connected via USB power
// -> don't use the VIN pin from Arduino!!??
// -> OR: Resolder it directly to the Power Jack?!
#define ENABLE_MOTOR 0

Freenove_WS2812B_Controller strip(0x20, /*leds_count=*/10, TYPE_GRB);
SCMD motorDriver;

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }
  strip.begin();
  strip.setAllLedsColor(0x00FF00); //set all LED color to gree
  
  motorDriver.settings.commInterface = I2C_MODE;
  motorDriver.settings.I2CAddress = 0x5D;  
  motorDriver.begin();
  motorDriver.enable();

  // TODO: Read location sensor too!
  
  // TODO: sonic sensor doesn't work yet as we only have 3.3V
  // -> maybe: Add the 5V voltage regulator in addition and output
  // -> it at the 5V pin of the arduino?!
  // -> Resolder the breakout board so that the +/- lines use the 5V!!
  //    And disconnect the i2c from there!
  DDRD = B01010101;
}

void loop() {
  Serial.println(motorDriver.ready());
  drive(255, 0);
  delay(1000);
  drive(-255, 0);
  delay(1000);
  drive(0, 255);
  delay(1000);
  drive(0, -255);
  delay(1000);
  drive(0, 0);
  delay(1000);  
  
//  Serial.println("---");
//  PORTD = 0;
//  Serial.println(PIND);
//  delayMicroseconds(5);
//  PORTD = B01010101;
//  Serial.println(PIND);
//  delayMicroseconds(10);
//  PORTD = 0;
//  Serial.println(PIND);
//
//  long startTimes[4] = {0};
//  long durations[4] = {0};
//    
//  Serial.println(PIND);
//  for (int i = 0; i<100; ++i) {
//    byte value = PIND;
//    long time = micros();
//    for (int b = 0; b<4; b++) {
//      byte bit = value & (b*2 + 1);
//      if (bit && startTimes[b] == 0) {
//        startTimes[b] = time;
//      }
//      if (!bit && startTimes[b] != 0 && durations[b] == 0) {
//        Serial.println("now!!");
//        durations[b] = time - startTimes[b];
//      }
//    }
//    delay(1);
//  }
//  Serial.println("----");
//  Serial.print(startTimes[0]);
//  Serial.print(" ");
//  Serial.print(startTimes[1]);
//  Serial.print(" ");
//  Serial.print(startTimes[2]);
//  Serial.print(" ");
//  Serial.print(startTimes[3]);
//  Serial.println();
}

void drive(int speedLeft, int speedRight) {
  if (!ENABLE_MOTOR) {
    return;
  }
  int absLeft = min(abs(speedLeft), 255);
  int absRight = min(abs(speedRight), 255);
  motorDriver.setDrive(0, speedLeft > 0, absLeft);
  motorDriver.setDrive(1, speedRight > 0, absRight);
}
