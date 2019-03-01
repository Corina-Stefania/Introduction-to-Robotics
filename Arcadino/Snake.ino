#include "LedControl.h"
#include <TFT.h>
#include <SPI.h>

#define CS_TFT   4
#define DC   24
#define RST  3
#define JOY_X A2 // joystick X axis pin
#define JOY_Y A3  // joystick Y axis pin

//driver MAX7219 pins
#define CS 10
#define CLK 11
#define DIN 12

// potentiometer for speed control
static const short potentiometer = A5;

// LED matrix brightness: between 0(darkest) and 15(brightest)
const short intensity = 2;

// initial snake length (1...256)
const short initialSnakeLength = 3;
// snake parameters
int snakeLength = initialSnakeLength;
int snakeSpeed = 1; // will be set according to potentiometer value
int snakeDirection = 0;
bool start = true;

// create an instance of the library
TFT TFTscreen = TFT(CS_TFT, DC, RST);
bool startMessage = false;

LedControl lc(DIN, CLK, CS, 4);

struct Point {
  int row = 0, col = 0;
  Point(int row = 0, int col = 0): row(row), col(col) {}
};

struct Coordinate {
  int x = 0, y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};

bool win = false;
bool gameOver = false;

// primary snake head coordinates, randomly generated
Point snake;

// food is not anywhere yet
Point food(-1, -1);

Coordinate joystickHome(500, 500);

// direction constants
const short up     = 1;
const short right  = 2;
const short down   = 3;
const short left   = 4;

// threshold where movement of the joystick will be accepted
const int joystickThreshold = 160;

// artificial logarithmity (steepness) of the potentiometer (-1 = linear, 1 = natural, bigger = steeper (recommended 0...1))
const float logarithmity = 0.4;

int age[16][16] = {};


void setup() {
  initialize();         // initialize pins & LED matrix
  calibrateJoystick(); // calibrate the joystick home (do not touch it)

  //initialize the library
  TFTscreen.begin();

  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  //set the text size
  TFTscreen.setTextSize(2);
  TFTscreen.setRotation(3);
  // set the text color to white
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("< or > to", 0, 10);
  TFTscreen.text("START", 0, 50);
}


void loop() {
  if (analogRead(A2) < 400 || analogRead(A2) > 650 && start == true) {
    start = false;
    showScoreMessage(snakeLength * 2);
  }
  if (start == false) {
    generateFood();    // if there is no food, generate one
    scanJoystick();    // watches joystick movements & blinks with food
    showScoreMessage(snakeLength * 2);
    calculateSnake();  // calculates snake parameters
    handleGameStates();
  }
}


// if there is no food, generate one and check for victory
void generateFood() {
  if (food.row == -1 || food.col == -1) {
    if (snakeLength >= 256) {
      win = true;
      return;
    }

    // generate food until it is in the right position
    do {
      food.col = random(16);
      food.row = random(16);
    } while (age[food.row][food.col] > 0);
  }
}


// custom inverse logarithm with variable steepness (logarithmity), see https://www.desmos.com/calculator/qmyqv84xis (input = 0...1)
float lnx(float n) {
  if (n < 0) return 0;
  if (n > 1) return 1;
  n = -log(-n * logarithmity + 1); // natural logarithm
  if (isinf(n)) n = lnx(0.999999); // prevent returning 'inf'
  return n;
}


// watches joystick movements & blinks with food
void scanJoystick() {
  int previousDirection = snakeDirection;
  // when the next frame will be rendered
  long timestamp = millis() + snakeSpeed;

  while (millis() < timestamp) {

    // calculate snake speed logarithmically (10...1000ms)
    float raw = mapf(analogRead(potentiometer), 0, 1023, 0, 1);
    snakeSpeed = mapf(lnx(raw), lnx(0), lnx(1), 10, 1000);
    // speed can't be 0
    if (snakeSpeed == 0) snakeSpeed = 1;

    // determine the direction of the snake
    analogRead( JOY_Y) < joystickHome.y - joystickThreshold ? snakeDirection = up    : 0;
    analogRead(  JOY_Y) > joystickHome.y + joystickThreshold ? snakeDirection = down  : 0;
    analogRead(  JOY_X) < joystickHome.x - joystickThreshold ? snakeDirection = left  : 0;
    analogRead(  JOY_X) > joystickHome.x + joystickThreshold ? snakeDirection = right : 0;

    // ignore directional change by 180 degrees
    snakeDirection + 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;
    snakeDirection - 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;

    // food blink
    if (food.row < 8 && food.col < 8)
      lc.setLed(0, food.row, food.col, millis() % 100 < 50 ? 1 : 0);
    else  if (food.row < 8 && 8 <= food.col < 16)
      lc.setLed(1, food.row % 8, food.col % 8, millis() % 100 < 50 ? 1 : 0);
    else  if (8 <= food.row < 16 && food.col < 8)
      lc.setLed(2, food.row % 8, food.col % 8, millis() % 100 < 50 ? 1 : 0);
    else  if (8 <= food.row < 16 && 8 <= food.col < 16)
      lc.setLed(3, food.row % 8, food.col % 8, millis() % 100 < 50 ? 1 : 0);
  }
}


// calculate snake movement
void calculateSnake() {
  switch (snakeDirection) {
    case up:
      snake.row--;
      fixEdge();
      break;

    case right:
      snake.col++;
      fixEdge();
      break;

    case down:
      snake.row++;
      fixEdge();
      break;

    case left:
      snake.col--;
      fixEdge();
      break;

    default:
      return;
  }
  if (snake.row < 8 && snake.col < 8)
    lc.setLed(0, snake.row, snake.col, 1);
  else if (snake.row < 8 && 8 <= snake.col < 16)
    lc.setLed(1, snake.row % 8, snake.col % 8, 1);
  else if (8 <= snake.row < 16 && snake.col < 8)
    lc.setLed(2, snake.row % 8, snake.col % 8, 1);
  else if (8 <= snake.row < 16 && 8 <= snake.col < 16)
    lc.setLed(3, snake.row % 8, snake.col % 8, 1);

  //if snake hits itself, the game is over
  if (age[snake.row][snake.col] != 0 && snakeDirection != 0) {
    gameOver = true;
    return;
  }

  // check if the food was eaten
  if (snake.row == food.row && snake.col == food.col) {
    snakeLength++;
    food.row = -1; // reset food
    food.col = -1;
  }
  
  updateAges();
  age[snake.row][snake.col]++;
}

// makes the snake go through walls
void fixEdge() {
  int width = 15;
  int height = 15;
  if (snake.col > width) {
    snake.col = 0;
  } else if (snake.col < 0) {
    snake.col = width;
  }
  if (snake.row > height) {
    snake.row = 0;
  } else if (snake.row < 0) {
    snake.row = height;
  }
}


// increment ages if all lit leds, turn off too old ones depending on the length of the snake
void updateAges() {
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 16; col++) {
      // if the led is lit, increment it's age
      if (age[row][col] > 0 ) {
        age[row][col]++;
      }

      // if the age exceeds the length of the snake, switch it off
      if (age[row][col] > snakeLength) {
        if (row < 8 && col < 8)
          lc.setLed(0, row, col, 0);
        else if (row < 8 && 8 <= col < 16)
          lc.setLed(1, row % 8, col % 8, 0);
        else if (8 <= row < 16 && col < 8)
          lc.setLed(2, row % 8, col % 8, 0);
        else if (8 <= row < 16 && 8 <= col < 16)
          lc.setLed(3, row % 8, col % 8, 0);
        age[row][col] = 0;
      }
    }
  }
}


void handleGameStates() {
  if (gameOver || win) {
    unrollSnake();

    // re-init the game
    win = false;
    gameOver = false;
    snake.row = random(16);
    snake.col = random(16);
    food.row = -1;
    food.col = -1;
    snakeLength = initialSnakeLength;
    snakeDirection = 0;
    memset(age, 0, sizeof(age[0][0]) * 16 * 16);
    lc.clearDisplay(0);
    lc.clearDisplay(1);
    lc.clearDisplay(2);
    lc.clearDisplay(3);
  }
}


void unrollSnake() {
  // switch off the food LED
  if (food.row < 8 && food.col < 8)
    lc.setLed(0, food.row, food.col, 0);
  else if (food.row < 8 && 8 <= food.col < 16)
    lc.setLed(1, food.row % 8, food.col % 8, 0);
  else if (8 <= food.row < 16 && food.col < 8)
    lc.setLed(2, food.row % 8, food.col % 8, 0);
  else if (8 <= food.row < 16 && 8 <= food.col < 16)
    lc.setLed(3, food.row % 8, food.col % 8, 0);

  for (int i = 1; i <= snakeLength; i++) {
    for (int row = 0; row < 16; row++) {
      for (int col = 0; col < 16; col++) {
        if (age[row][col] == i) {
          if (row < 8 && col < 8)
            lc.setLed(0, row, col, 0);
          else if (row < 8 && 8 <= col < 16)
            lc.setLed(1, row % 8, col % 8, 0);
          else if (8 <= row < 16 && col < 8)
            lc.setLed(2, row % 8, col % 8, 0);
          else if (8 <= row < 16 && 8 <= col < 16)
            lc.setLed(3, row % 8, col % 8, 0);
        }
      }
    }
  }
}


// calibrate the joystick home for 10 times
void calibrateJoystick() {
  Coordinate values;

  for (int i = 0; i < 10; i++) {
    values.x += analogRead(  JOY_X);
    values.y += analogRead(  JOY_Y);
  }

  joystickHome.x = values.x / 10;
  joystickHome.y = values.y / 10;
}


void initialize() {
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, intensity); // sets brightness (0~15 possible values)
  lc.clearDisplay(0); // clear screen

  lc.shutdown(1, false);
  lc.setIntensity(1, intensity);
  lc.clearDisplay(1);

  lc.shutdown(2, false);
  lc.setIntensity(2, intensity);
  lc.clearDisplay(2);

  lc.shutdown(3, false);
  lc.setIntensity(3, intensity);
  lc.clearDisplay(3);

  randomSeed(analogRead(A5));
  snake.row = random(16);
  snake.col = random(16);
}


void deleteStartMessage() {
  TFTscreen.background(0, 0, 0);
}

void showScoreMessage(int score) {
  if (startMessage == false)
  {
    startMessage = true;
    deleteStartMessage();
  }
  TFTscreen.stroke(0, 0, 0);
  TFTscreen.rect(75, 57, 50, 50);
  TFTscreen.fill(0, 0, 0);

  char buff[20];
  itoa(score, buff, 10);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("Score: ", 6, 57);
  TFTscreen.text(buff, 75, 57);
}


// standard map function, but with floats
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
