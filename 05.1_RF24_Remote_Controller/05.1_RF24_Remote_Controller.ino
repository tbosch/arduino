#include <TFT_22_ILI9225.h>
#include <SPI.h>
#include "RF24.h"

RF24 radio(9, 10);                // define the object to control NRF24L01
const byte addresses[6] = {"Free1"};
// wireless communication
int dataWrite[8];                 // define array used to save the write data
int dataRead[8];                 // define array used to save the read data
// pin
const int pot1Pin = A0,           // define POT1 Potentiometer
          pot2Pin = A1,           // define POT2 Potentiometer
          joystickXPin = A2,      // define pin for direction X of joystick
          joystickYPin = A3,      // define pin for direction Y of joystick
          joystickZPin = 7,       // define pin for direction Z of joystick
          s1Pin = 4,              // define pin for S1
          s2Pin = 3,              // define pin for S2
          s3Pin = 2,              // define pin for S3
          led1Pin = 6,            // define pin for LED1 which is close to POT1 and used to indicate the state of POT1
          led2Pin = 5,            // define pin for LED2 which is close to POT2 and used to indicate the state of POT2
          led3Pin = 8;            // define pin for LED3 which is close to NRF24L01 and used to indicate the state of NRF24L01

int trimX;
int trimY;

#define TFT_RST 0
#define TFT_RS  A4
#define TFT_CS  A5  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 0   // 0 if wired to +5V directly
#define TFT_BRIGHTNESS 200 // Initial brightness of TFT backlight (optional)

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

/*
 * Tux black/white image in 40x40 converted using Ardafruit bitmap converter
 * https://github.com/ehubin/Adafruit-GFX-Library/tree/master/Img2Code
 */
static const uint8_t PROGMEM car[] = 
{
0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,
0x3,0xfe,0x0,0x7f,0xc0,
0x75,0x25,0x7e,0xa4,0xae,
0x74,0x93,0x7e,0x92,0x6e,
0xf6,0x49,0x7e,0xc9,0x2f,
0xf5,0x25,0x7e,0xa4,0xaf,
0xf4,0x93,0x7e,0x92,0x6f,
0xf6,0x49,0x7e,0xc9,0x2f,
0xf3,0xfe,0x7e,0x7f,0xcf,
0xf1,0xfc,0x7e,0x3f,0x8f,
0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,
0xfd,0xff,0xff,0xff,0xff,
0xf9,0xff,0xff,0xff,0xff,
0xf1,0xff,0xff,0xff,0xff,
0xe1,0xff,0xff,0xff,0xff,
0xc0,0x0,0xff,0xff,0xff,
0x80,0x0,0xff,0xff,0xff,
0x0,0x0,0xff,0xff,0xff,
0x0,0x0,0xff,0xff,0xff,
0x80,0x0,0xff,0xff,0xff,
0xc0,0x0,0xff,0xff,0xff,
0xe1,0xff,0xff,0xff,0xff,
0xf1,0xff,0xff,0xff,0xff,
0xf9,0xff,0xff,0xff,0xff,
0xfd,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,
0xf1,0xfc,0x7e,0x3f,0x8f,
0xf3,0xfe,0x7e,0x7f,0xcf,
0xf6,0x49,0x7e,0xc9,0x2f,
0xf4,0x93,0x7e,0x92,0x6f,
0xf5,0x25,0x7e,0xa4,0xaf,
0xf6,0x49,0x7e,0xc9,0x2f,
0x74,0x93,0x7e,0x92,0x6e,
0x75,0x25,0x7e,0xa4,0xae,
0x3,0xfe,0x0,0x7f,0xc0,
0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0
};


void setup() {
  Serial.begin(9600);
  // NRF24L01
  radio.begin();                      // initialize RF24
  radio.setPALevel(RF24_PA_MAX);      // set power amplifier (PA) level
  radio.setDataRate(RF24_1MBPS);      // set data rate through the air
  radio.setRetries(15, 15);            // set the number and delay of retries
//  radio.enableAckPayload();                     // Allow optional ack payloads
  radio.openWritingPipe(addresses);   // open a pipe for writing
  radio.stopListening();              // stop listening for incoming messages
  
// pin
  pinMode(joystickZPin, INPUT);       // set led1Pin to input mode
  pinMode(s1Pin, INPUT);              // set s1Pin to input mode
  pinMode(s2Pin, INPUT);              // set s2Pin to input mode
  pinMode(s3Pin, INPUT);              // set s3Pin to input mode
  pinMode(led1Pin, OUTPUT);           // set led1Pin to output mode
  pinMode(led2Pin, OUTPUT);           // set led2Pin to output mode
  pinMode(led3Pin, OUTPUT);           // set led3Pin to output mode

  // initialyze trim
  trimX = analogRead(joystickXPin) - 512;
  trimY = analogRead(joystickYPin) - 512;
  Serial.print(">>> Init. X:");
  Serial.print(analogRead(joystickXPin));
  Serial.print(" Y:");
  Serial.print(analogRead(joystickYPin));
  Serial.println();

  tft.begin();
  // TODO: Print car image instead of tux and in a smaller version.
//  tft.fillRectangle(0, 0, tft.maxX() - 1, tft.maxY() - 1, COLOR_WHITE);
//  tft.setBackgroundColor(COLOR_WHITE);
  tft.clear();
//  tft.setBackgroundColor(COLOR_BLACK);
  tft.drawBitmap(tft.maxX()/2-20, tft.maxY()/2-20, car, 40, 40, COLOR_WHITE);  
//  tft.clear();
}

void loop()
{
  int x = analogRead(joystickXPin) - trimX;
  int y = analogRead(joystickYPin) - trimY;
  int s3 = digitalRead(s3Pin);
  if(s3 == 0){
    x = limitSpeed(x);
    y = limitSpeed(y);
  }
  
  if (true) {
    Serial.print(">>> X:");
    Serial.print(x);
    Serial.print(" Y:");
    Serial.print(y);
    Serial.print(" S3:");
    Serial.print(s3);
    Serial.println();
  }

  // put the values of rocker, switch and potentiometer into the array
  dataWrite[0] = analogRead(pot1Pin); // save data of Potentiometer 1
  dataWrite[1] = analogRead(pot2Pin); // save data of Potentiometer 2
  dataWrite[2] = x;  // save data of direction X of joystick
  dataWrite[3] = y;  // save data of direction Y of joystick
  dataWrite[4] = digitalRead(joystickZPin); // save data of direction Z of joystick
  dataWrite[5] = digitalRead(s1Pin);        // save data of switch 1
  dataWrite[6] = digitalRead(s2Pin);        // save data of switch 2
  dataWrite[7] = s3;        // save data of switch 3

  // write radio data
  if (radio.writeFast(dataWrite, sizeof(dataWrite)))
  {
    digitalWrite(led3Pin, HIGH);
//    if (radio.available()) {         // read all the data
//      radio.read(dataRead, sizeof(dataRead));   // read data
//    }
  }
  else {
    digitalWrite(led3Pin, LOW);
  }
  delay(20);

  // make LED emit different brightness of light according to analog of potentiometer
  analogWrite(led1Pin, map(dataWrite[0], 0, 1023, 0, 255));
  analogWrite(led2Pin, map(dataWrite[1], 0, 1023, 0, 255));

  // TODO: Show sonic distances around the car.
//  tft.drawLine(random(tft.maxX()), random(tft.maxY()), random(tft.maxX()), random(tft.maxY()), random(0xffff));
  
}

int limitSpeed(int speed) {
  if (speed < 312){
    return 312;
  }
  if (speed > 712){
    return 712;
  }
  return speed;
}
