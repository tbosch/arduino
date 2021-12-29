#include "RF24_Remote.h"

RF24 radio(PIN_SPI_CE, PIN_SPI_CSN);
const byte address[6] = {"Bosch"};
int nrfDataRead[8];
int nrfDataWrite[8] = {0};
bool nrfComplete = false;

bool nrf24L01Setup() {
  // NRF24L01
  if (radio.begin()) {
    // TODO: Use RF24_PA_HIGH if the range is not enough.
    radio.setPALevel(RF24_PA_LOW);
    radio.setRetries(15, 15);
    radio.enableAckPayload();                     // Allow optional ack payloads
    radio.openReadingPipe(1, address);
    radio.startListening();
    radio.writeAckPayload(1, nrfDataWrite, sizeof(nrfDataWrite));          // Pre-load an ack-paylod into the FIFO buffer for pipe 1

    FlexiTimer2::set(20, 1.0 / 1000, checkNrfReceived); // call every 20 1ms "ticks"
    FlexiTimer2::start();

    return true;
  }
  return false;
}

void checkNrfReceived() {
  delayMicroseconds(1000);
  if (radio.available()) {             // if receive the data
    radio.read(nrfDataRead, sizeof(nrfDataRead));   // read data
    radio.writeAckPayload(1, nrfDataWrite, sizeof(nrfDataWrite) );
    nrfComplete = true;
    return;
  }
  nrfComplete = false;
}

bool getNrf24L01Data()
{
  return nrfComplete;
}

void clearNrfFlag() {
  nrfComplete = 0;
}

void updateCarActionByNrfRemote() {
  int x = nrfDataRead[2] - 512;
  int y = nrfDataRead[3] - 512;
  int pwmL, pwmR;
  if (y < 0) {
    pwmL = (-y + x) / 2;
    pwmR = (-y - x) / 2;
  }
  else {
    pwmL = (-y - x) / 2;
    pwmR = (-y + x) / 2;
  }
  motorRun(pwmL, pwmR);

  if (nrfDataRead[4] == 0) {
    setBuzzer(true);
  }
  else {
    setBuzzer(false);
  }
}

void resetNrfDataBuf() {
  nrfDataRead[0] = 0;
  nrfDataRead[1] = 0;
  nrfDataRead[2] = 512;
  nrfDataRead[3] = 512;
  nrfDataRead[4] = 1;
  nrfDataRead[5] = 1;
  nrfDataRead[6] = 1;
  nrfDataRead[7] = 1;
}

u8 updateNrfCarMode() {
  // nrfDataRead [5 6 7] --> 111
  return ((nrfDataRead[5] == 1 ? 1 : 0) << 2) | ((nrfDataRead[6] == 1 ? 1 : 0) << 1) | ((nrfDataRead[7] == 1 ? 1 : 0) << 0);
}
