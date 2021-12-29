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

#define NRF_UPDATE_TIMEOUT    1000

#define MCP23017_ADDRESS 0x27
// This is before we switch to BANK=1 mode. After that, this
// register lives at 0x05
#define MCP23017_IOCON 0x0A
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x10
#define MCP23017_GPIOA 0x09
#define MCP23017_GPIOB 0x19

// Note: This can't be higher than BUFFER_LENGTH from Wire.h!
#define SONIC_READ_COUNT 32
// 10ms = 3.3m of sound.
// Don't set this too high so the loop is not blocked for too long.
#define SONIC_READ_TIME_US 10000
#define SONIC_SPEED_CM_PER_US 0.033

u32 lastNrfUpdateTime = 0;
u32 lastBeepUpdateTime = 0;
float sonicResolution = 0;
float sonicMinDistance = 0;
float sonicDistances[8] = {0};

void setup() {
  Serial.begin(9600);  
  Wire.begin();
  // Note: This doesn't work with the LED strip anymore :-(
  // We set this to get a higher accuracy from the sonic sensors.
  // (from about 250us down to 120us per read loop).
  Wire.setClock(400000);
  
  pinsSetup();
  if (!nrf24L01Setup()) {
    alarm(4, 2);
  }

  initSonic();
}

// TODO: Show sonic distances on the client display as top down view.
// TODO: Also read the location sensor too!
// -> show a compass on the client display!
// TODO: Get slower the closer we get to a wall
// TODO: Allow to continue driving when one of the buttons is pressed.
// 
void loop() {  
  readSonicSensor();
  if (/*debug=*/false) {
    Serial.print(">>> min distance in cm:");
    Serial.print(sonicMinDistance);
    Serial.print(" resolution in cm:");
    Serial.print(sonicResolution);
    Serial.println();
  }
  if (/*debug=*/false) {
    Serial.print(">>> distances in cm:");
    for (int b=0; b<8; ++b) {
      Serial.print(sonicDistances[b]);
      Serial.print(" ");
    }
    Serial.println();
  }

  if (sonicMinDistance < 80){
    setBuzzer(true);
    if( nrfDataRead[7] == 1) {
      motorRun(0, 0);
      return;
    }
  } else {
    setBuzzer(false);   
  }
 
  
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
}

void readSonicSensor() {
  // make trigPin output high level lasting for 10μs to triger HC_SR04
  writeIOExpansionRegister(MCP23017_GPIOA, 0xFF);
  delayMicroseconds(10);
  writeIOExpansionRegister(MCP23017_GPIOA, 0x00);
  
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(MCP23017_GPIOB);
  Wire.endTransmission();
  long sonicStartTimes[8] = {0};
  long sonicEndTimes[8] = {0};
  long currTime = micros();
  long loopStartTime = micros();
  long loopCount = 0;
  for (long currTime = loopStartTime; currTime - loopStartTime < SONIC_READ_TIME_US; currTime = micros(), ++loopCount) {
    Wire.requestFrom(MCP23017_ADDRESS, 1, /*sendStop=*/false);
    uint8_t data = Wire.read();
    for (int b=0; b<8; ++b) {
      bool isSet = data & (1 << b);
      if (sonicStartTimes[b] == 0 && isSet) {
        sonicStartTimes[b] = currTime;
      } else if (sonicStartTimes[b] > 0 && sonicEndTimes[b] == 0 && !isSet) {
        sonicEndTimes[b] = currTime;
      }
    }
  }
  sonicResolution = (micros() - loopStartTime) * SONIC_SPEED_CM_PER_US / loopCount;
  
  sonicMinDistance = SONIC_READ_TIME_US * SONIC_SPEED_CM_PER_US;
  for (int b=0; b<8; ++b) {
    long duration = SONIC_READ_TIME_US;
    if (sonicEndTimes[b] > 0) {
      duration = sonicEndTimes[b] - sonicStartTimes[b];
    }
    float distanceCm = duration * SONIC_SPEED_CM_PER_US;     
    // Average over the last 3 runs to reduce flakiness.
    sonicDistances[b] = (sonicDistances[b]*2 + distanceCm) / 3;    
    sonicMinDistance = min(sonicMinDistance, sonicDistances[b]);
  }
}

void writeIOExpansionRegister(uint8_t regAddr, uint8_t regValue){
  Wire.beginTransmission(MCP23017_ADDRESS);
  Wire.write(regAddr);
  Wire.write(regValue);
  Wire.endTransmission();
}
