#include <Wire.h>

/**********************************************************************
  Filename    : RF24_Remote_Car.ino
  Product     : Freenove 4WD Car for UNO
  Description : A RF24 Remote Car.
  Auther      : www.freenove.com
  Modification: 2020/11/27
**********************************************************************/
#include "Freenove_4WD_Car_for_Arduino.h"
#include "RF24_Remote.h"

#define DEBUG 0
#define NRF_UPDATE_TIMEOUT    1000

#define MCP23017_ADDRESS 0x27
// This is before we switch to BANK=1 mode. After that, this
// register lives at 0x05
#define MCP23017_IOCON 0x0A
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x10
#define MCP23017_GPIOA 0x09
#define MCP23017_GPIOB 0x19
#define MCP23017_INTCONB 0x14
#define MCP23017_DEFVALB 0x13
#define MCP23017_GPINTENB 0x12
#define MCP23017_INTFB 0x17
#define MCP23017_INTCAPB 0x18

#define MCP23017_INT_ARDUINO_PIN 2

// 6ms = approx. 2m of sound.
// Don't set this too high so the loop is not blocked for too long.
#define SONIC_READ_TIME_US 6000
#define SONIC_SPEED_CM_PER_US 0.033

u32 lastNrfUpdateTime = 0;
u32 lastBeepUpdateTime = 0;
float sonicResolution = 0;
volatile long sonicInterruptTime = 0;

void setup() {
  if (DEBUG) {
    Serial.begin(9600);  
  }
  Wire.begin();
  
  pinsSetup();
  if (!nrf24L01Setup()) {
    alarm(4, 2);
  }

  initSonic();
}

// TODO: Also read the location sensor too!
// -> show a compass on the client display!
// TODO: Enable the LED strip again too!
void loop() {
  long measureStartTime = micros();  
  for (int i=0; i<8; ++i) {
    float distance = readSonicSensor(i);
    if (distance > 0) {
      nrfDataWrite[i] = readSonicSensor(i);
    }
  }
  long measureDuration = micros() - measureStartTime;
  if (DEBUG) {
    Serial.print(" measure time:");
    Serial.print(measureDuration / 1000);
    Serial.print(" distances in cm:");
    for (int i=0; i<8; ++i) {
      Serial.print(nrfDataWrite[i]);
      Serial.print(" ");
    }
    Serial.println();
  }

  // TODO: Do this on the remote!
//  if (sonicMinDistance < 100){
//    setBuzzer(true);
//    if( nrfDataRead[7] == 1) {
//      motorRun(0, 0);
//      return;
//    }
//  } else {
//    setBuzzer(false);   
//  }
 
  
  if (getNrf24L01Data()) {
    clearNrfFlag();
    updateCarActionByNrfRemote();
    lastNrfUpdateTime = millis();
  }
  if (millis() - lastNrfUpdateTime > NRF_UPDATE_TIMEOUT) {
    lastNrfUpdateTime = millis();
    resetNrfDataBuf();
    updateCarActionByNrfRemote();
  }  
}

void sonicInterrupt() {
  sonicInterruptTime = micros();
}

void initSonic() {
  // Set BANK bit = Enable 8 bit mode. This allows us to read a single bank,
  // which is faster.
  // Set SEQOP bit = Enable byte mode.
  // This disables automatic increment of addresses when reading.
  // We use that for polling.
  writeIOExpansionRegister(MCP23017_IOCON, 0xA0);

  // Mark all A pins as output.
  writeIOExpansionRegister(MCP23017_IODIRA,0x00);
  // Mark all B pins as input.
  writeIOExpansionRegister(MCP23017_IODIRB,0xff);  

  // Setup to trigger interrupt on falling value.
  // set the pin interrupt control:
  // 1 = compare against given value.
  writeIOExpansionRegister(MCP23017_INTCONB,0xff);  
  // Set default value to 1, different value (=0) triggers interrupt.
  writeIOExpansionRegister(MCP23017_DEFVALB,0xff);
  // Setup a callback for the arduino INT handler.
  pinMode(MCP23017_INT_ARDUINO_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(MCP23017_INT_ARDUINO_PIN),sonicInterrupt,FALLING);
}

float readSonicSensor(int idx) {
  if (idx<0 || idx >7) {
    return 0;
  }
  // Enable interrupts only for that sensor.
  writeIOExpansionRegister(MCP23017_GPINTENB,1<<idx);

  // make trigPin output high level lasting for 10μs to triger HC_SR04
  // Note: Somehow I wired the trigger and sensor pins in the opposite directions!
  // -> 0x80 trigger on bank A = 0x01 sensor on bank B.
  writeIOExpansionRegister(MCP23017_GPIOA, 1<<(7-idx));
  delayMicroseconds(10);
  writeIOExpansionRegister(MCP23017_GPIOA, 0x00);

  // Wait for the high value.
  // This is also clearing the interrupt value so we can receive further interrupts.  
  long startTime = 0;
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_GPIOB);
  Wire.endTransmission();
  for (int i=0; i<10 && startTime == 0; i++) {
    Wire.requestFrom(MCP23017_ADDRESS, 1, /*sendStop=*/false);  
    if (Wire.read() > 0) {
      startTime = micros();
    }
  }
  if (startTime == 0) {
    // We were too slow to capture the pulse.
    return 0;
  }
  sonicInterruptTime = 0;
  delayMicroseconds(SONIC_READ_TIME_US);
  
  if (sonicInterruptTime == 0) {
    return SONIC_READ_TIME_US * SONIC_SPEED_CM_PER_US;
  }    
  return (sonicInterruptTime - startTime) * SONIC_SPEED_CM_PER_US;
}

void writeIOExpansionRegister(uint8_t regAddr, uint8_t regValue){
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(regAddr);
  Wire.write(regValue);
  Wire.endTransmission();
}
