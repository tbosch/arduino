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
  BLANK = 0,
  FRUIT1 = 1,
  FRUIT2 = 2,
  RESERVED4 = 3,
  RESERVED5 = 4,  
  SNAKE_HEAD = 5,
};

int maxX, maxY;
GameMode gameMode;
bool wasJoystickDown;
Cell board[BOARD_WIDTH][BOARD_HEIGHT] = {};
Direction joystickDir;
Direction snakeLastMoveDir;
byte snakeMaxLength;
long snakeLastMoveTime;
int snakeWaitTime;

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

  showScreen("Welcome to Snake!");
}

void loop()
{
  
  Direction dir = readJoystickDir();
  bool isClick = readJoystickClick();

  if (gameMode == RUNNING) {
    moveSnake(dir);
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
  snakeLastMoveDir = RIGHT;
  joystickDir = RIGHT;
  snakeMaxLength = 3;
  snakeWaitTime = 300;
  snakeLastMoveTime = millis();
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      board[x][y] = BLANK;
    }
  }
  updateCell(maxX/2, maxY/2, SNAKE_HEAD);
  for (byte i=0; i<5; ++i) {
    updateCell(random(0, maxX+1), random(0, maxY+1), FRUIT1);
  }
  for (byte i=0; i<5; ++i) {
    updateCell(random(0, maxX+1), random(0, maxY+1), FRUIT2);
  }
}

void moveSnake(Direction dir) {
  long now = millis();
  if ((now - snakeLastMoveTime) <= snakeWaitTime) {
    return;
  }
  if ((dir == LEFT && snakeLastMoveDir == RIGHT) || 
      (dir == RIGHT && snakeLastMoveDir == LEFT) ||
      (dir == UP && snakeLastMoveDir == DOWN) ||
      (dir == DOWN && snakeLastMoveDir == UP)) {
    // Don't allow 180 degree changes.
    dir = snakeLastMoveDir;
  }
  snakeLastMoveDir = dir;
  snakeLastMoveTime = now;
  byte headX = 0;
  byte headY = 0;
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      if (board[x][y] == SNAKE_HEAD) {
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
  byte newHeadCell = board[newHeadX][newHeadY];
  if (newHeadCell >= SNAKE_HEAD) {
    showScreen("Game over!");
    return;    
  }
  if (newHeadCell == FRUIT1) {
    snakeMaxLength += 3;
  }
  if (newHeadCell == FRUIT2) {
    snakeWaitTime = snakeWaitTime * 0.75;
  }
  
  // Update the cells.
  byte fruitCount = 0;
  for (byte x=0; x<BOARD_WIDTH; ++x) {
    for (byte y=0; y<BOARD_HEIGHT; ++y) {
      byte cell = board[x][y];
      if (x == newHeadX && y == newHeadY) {
        updateCell(x, y, SNAKE_HEAD);
      } else if (cell >= SNAKE_HEAD) {
        if (cell - SNAKE_HEAD >= snakeMaxLength) {
          cell = BLANK;
        } else {
          cell++;
        }
        updateCell(x, y, cell);
      } else if (cell > 0) {
        fruitCount++;
      }
    }
  }
  if (fruitCount == 0) {
    showScreen("You win!");    
  }
}

void updateCell(int x, int y, Cell cell) {
  board[x][y] = cell;
  // Note: coordinates are inclusive!
  if (cell == BLANK) {
    tft.fillRectangle(x*10, y*10, x*10+9, y*10+9, COLOR_BLACK);    
  } else if (cell == SNAKE_HEAD) {
    tft.fillRectangle(x*10, y*10, x*10+9, y*10+9, COLOR_WHITE);
  } else if (cell == FRUIT1) {
    tft.fillRectangle(x*10, y*10, x*10+9, y*10+9, COLOR_RED);
  } else if (cell == FRUIT2) {
    tft.fillRectangle(x*10, y*10, x*10+9, y*10+9, COLOR_BLUE);
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
    if (joystickX < -100) {
      joystickDir = LEFT;
    } else if (joystickX > 100) {
      joystickDir = RIGHT;
    }    
  } else {
    if (joystickY < -100) {
      joystickDir = UP;
    } else if (joystickY > 100) {
      joystickDir = DOWN;
    }    
  }
  return joystickDir;
}

bool readJoystickClick() {
  bool isJoystickDown = !digitalRead(joystickZPin);
  bool isClick = wasJoystickDown && !isJoystickDown;
  wasJoystickDown = isJoystickDown;
  return isClick;  
}
