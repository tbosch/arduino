#include <TFT_22_ILI9225.h>
#include <SPI.h>

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

#define DEBUG true
#define TFT_RST 0
#define TFT_RS  A4
#define TFT_CS  A5  // SS
#define TFT_SDI 11  // MOSI
#define TFT_CLK 13  // SCK
#define TFT_LED 0   // 0 if wired to +5V directly
#define TFT_BRIGHTNESS 200 // Initial brightness of TFT backlight (optional)

#define BOARD_WIDTH 22
#define BOARD_HEIGHT 18

// Use hardware SPI (faster - on Uno: 13-SCK, 12-MISO, 11-MOSI)
TFT_22_ILI9225 tft = TFT_22_ILI9225(TFT_RST, TFT_RS, TFT_CS, TFT_LED, TFT_BRIGHTNESS);

enum GameMode {
  STOPPED,
  RUNNING,
};

enum Direction {
  LEFT,
  RIGHT,
  UP,
  DOWN
};

enum Cell {
  DRAW_BIT = 1<<7,
  DATA_BITMASK = ~DRAW_BIT,
  RESERVED1 = 1,
  RESERVED2 = 2,
  RESERVED4 = 3,
  RESERVED5 = 4,  
  SNAKE_HEAD = 5,
};

int maxX, maxY;
GameMode gameMode;
int lastTick;
bool wasJoystickDown;
Direction lastDir;
Cell board[BOARD_WIDTH][BOARD_HEIGHT] = {};

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }
  // pin
  pinMode(joystickXPin, INPUT);
  pinMode(joystickYPin, INPUT);
  pinMode(joystickZPin, INPUT);
  pinMode(s1Pin, INPUT);              // set s1Pin to input mode
  pinMode(s2Pin, INPUT);              // set s2Pin to input mode
  pinMode(s3Pin, INPUT);              // set s3Pin to input mode
  pinMode(led1Pin, OUTPUT);           // set led1Pin to output mode
  pinMode(led2Pin, OUTPUT);           // set led2Pin to output mode
  pinMode(led3Pin, OUTPUT);           // set led3Pin to output mode

  trimX = analogRead(joystickXPin);
  trimY = analogRead(joystickYPin);

  tft.begin();
  tft.setOrientation(1);
  tft.setFont(Terminal6x8);

  maxX = tft.maxX() / 10 - 1;
  maxY = tft.maxY() / 10 - 1;

  lastTick = currentTick();

  showScreen("Welcome to Snake!");
}

void loop()
{
  
  Direction dir = readJoystickDir();
  bool isClick = readJoystickClick();
  bool ticked = tickChanged();

  if (gameMode == RUNNING && ticked) {
    moveSnake(dir);
    drawBoard();
  } else if (gameMode == STOPPED && isClick) {
    startGame();
  }
}

void showScreen(String msg) {
  tft.drawText(5, tft.maxY() / 2, msg, COLOR_WHITE);

  gameMode = STOPPED;  
}

void startGame() {
  tft.clear();
  gameMode = RUNNING;
  lastDir = RIGHT;
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      board[x][y] = DRAW_BIT;
    }
  }
  board[maxX/2][maxY/2] |= SNAKE_HEAD;
}

void moveSnake(Direction dir) {
  byte headX = 0;
  byte headY = 0;
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      if ((board[x][y] & DATA_BITMASK) == SNAKE_HEAD) {
        headX = x;
        headY = y;
        break;        
      }
    }
  }
  byte newHeadX = addXDir(headX, dir);
  byte newHeadY = addYDir(headY, dir);
  if (newHeadX < 0 || newHeadX > maxX || newHeadY < 0 || newHeadY > maxY) {
    showScreen("Game over!");
    return;
  }
  byte newHeadCell = board[newHeadX][newHeadY] & DATA_BITMASK;
  if (newHeadCell >= SNAKE_HEAD) {
    showScreen("Game over!");
    return;    
  }
  
  // Clear the old tail / update the parts.  
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      byte cell = board[x][y] & DATA_BITMASK;
      if (cell >= SNAKE_HEAD) {
        if (cell - SNAKE_HEAD >= 6) {
          cell = DRAW_BIT;
        } else {
          // Increment data but no new draw.
          cell++;
        }
        board[x][y] = cell;
      }
    }
  }
  board[newHeadX][newHeadY] = SNAKE_HEAD | DRAW_BIT;
}

void drawBoard() {
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      Cell& cell = board[x][y];
      if ((cell & DRAW_BIT) > 0) {        
        cell &= ~DRAW_BIT;
        drawCell(x, y, cell);
      }
    }
  }
}

void drawCell(int x, int y, Cell cell) {
  // Note: coordinates are inclusive!
  if (cell == 0) {
    tft.fillRectangle(x*10, y*10, x*10+9, y*10+9, COLOR_BLACK);    
  } else {
    tft.fillRectangle(x*10, y*10, x*10+9, y*10+9, COLOR_WHITE);
  }
}

byte addXDir(byte x, Direction dir) {
  switch (dir) {
    case LEFT:
      return x-1;
    case RIGHT:
      return x+1;
    default:
      return x;
  }  
}

byte addYDir(byte y, Direction dir) {
  switch (dir) {
    case UP:
      return y-1;
    case DOWN:
      return y+1;
    default:
      return y;
  }  
}

Direction readJoystickDir() {
  int joystickX = trimX - analogRead(joystickXPin);
  int joystickY = trimY - analogRead(joystickYPin);
  
  if (abs(joystickX) > abs(joystickY)) {
    if (joystickX < -100 && lastDir != RIGHT) {
      lastDir = LEFT;
    } else if (joystickX > 100 && lastDir != LEFT) {
      lastDir = RIGHT;
    }    
  } else {
    if (joystickY < -100 && lastDir != DOWN) {
      lastDir = UP;
    } else if (joystickY > 100 && lastDir != UP) {
      lastDir = DOWN;
    }    
  }  
  return lastDir;
}

bool readJoystickClick() {
  bool isJoystickDown = !digitalRead(joystickZPin);
  bool isClick = wasJoystickDown && !isJoystickDown;
  wasJoystickDown = isJoystickDown;
  return isClick;  
}

bool tickChanged() {
  int tick = currentTick();
  bool changed = tick != lastTick;
  lastTick = tick;
  return changed;  
}

int currentTick() {
  return millis() / 200;
}
