#include "LedControl.h"
#include <TFT.h>  
#include <SPI.h>

#define JOY_X A2
#define JOY_Y A3
#define SIZE 16
#define JoyButton 2

#define CS   4
#define DC   24
#define RST  3

#define DIN 12
#define CLK 11
#define LOAD 10

LedControl lc = LedControl(DIN, CLK, LOAD, 4);
TFT TFTscreen = TFT(CS, DC, RST);


unsigned int Matrix[SIZE][SIZE];

//game lost display
bool sadFace[][16] = // cuttest sad face ever
{
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
  {1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1},
  {1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

//Enemies
typedef enum {
  ACTIVE, INACTIVE
} EnemyState;

typedef struct {
  int x;
  float y;
} Enemy;


Enemy enemies[3];

struct Coordinate {
  int x = 0, y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};

// game variables
unsigned long lastUpdate = 0;
unsigned long deadTime = millis(); // time that the player dies
bool playerAlive = false;

int heroX; //start the hero in the center of the screen
float vSpeed;
bool pressed; //indicates if there is a button pressed
int enemyX[3] = {3, 8, 10};
float enemyY[3] = {0, -8, -12};

// threshold where movement of the joystick will be accepted
const int joystickThreshold = 160;
// construct with default values in case the user turns off the calibration
Coordinate joystickHome(500, 500);

int dir; //player direction. 0 keep pos, -1 go left, +1 go right
int score =0;


void setup() {
  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, 2); // sets brightness (0~15 possible values)
  lc.clearDisplay(0);// clear screen

  lc.shutdown(1, false); 
  lc.setIntensity(1, 2); 
  lc.clearDisplay(1);

  lc.shutdown(2, false);
  lc.setIntensity(2, 2); 
  lc.clearDisplay(2);

  lc.shutdown(3, false);
  lc.setIntensity(3, 2);
  lc.clearDisplay(3);

  //initialize the library
  TFTscreen.begin();

  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  
  //set the text size
  TFTscreen.setTextSize(2);
  TFTscreen.setRotation(3);
  
  // set the text color to white
  TFTscreen.stroke(255,255,255); 

  randomSeed(analogRead(A6));

  clearMatrix();
  setupInitialVariables();
  calibrateJoystick();
  drawMatrix();
}

void loop() {
  int refreshInterval = 25;
      if ((millis() - lastUpdate) >= refreshInterval) {
        lastUpdate = millis();
        if (playerAlive) {
            updatePlayerColumn();
            updateEnemyRow();
          displayOnLcd();
          drawMatrix();
          drawEnemies();
          checkLevel();
          displayMatrix();
          checkCollisions();
        } else {
          checkActions();
        }
      }
}


void updatePlayerColumn(){
  int joyRead = analogRead(JOY_X);
  if (joyRead < 400) {          //left
    heroX = (heroX + SIZE - 1) % SIZE;
  }
  if (joyRead > 650) {        //right
    heroX = (heroX + 1) % SIZE;
  }
  score++;
}

/** 
  Enemy looks like:

         ***
         ***
          *
*/
void drawEnemies() {
  int y;
  for (byte i = 0; i < 3; i++) {
    Enemy e =  enemies[i];
    y = e.y;
        if (y >= 0 && y <=15) {
          Matrix[(int)e.y][e.x] = 1;
        }
        y--;

        if (y >= 0 && y <=15) {
          Matrix[y][e.x] = 1;
          Matrix[y][e.x - 1] = 1;
          Matrix[y][e.x + 1] = 1;
        }
        y--;

        if (y >= 0 && y <= 15) {
          Matrix[y][e.x] = 1;
          Matrix[y][e.x - 1] = 1;
          Matrix[y][e.x + 1] = 1;
        }
    }
}


void updateEnemyRow() {
   for (byte e = 0; e < 3; e++) { 
    enemies[e].y += vSpeed;   
    if (enemies[e].y > 18) {
      enemies[e].y = -1;
      if (random(3))
        enemies[e].x = 5;
        else enemies[e].x = 13;
    }
  }
}

void setupInitialVariables() {
  heroX = 5;
  score = 0;
  playerAlive = false;
  pressed = false;
  vSpeed = 0.5;
  dir = 0;

  enemies[0].x = 5;
  enemies[0].y = 0;

  enemies[1].x = 8;
  enemies[1].y = -8;

  enemies[1].x = 10;
  enemies[1].y = -12;
}

void clearMatrix() {
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      Matrix[i][j] = 0;
    }
  }
}


void displayMatrix()
{
  for (int i = 0; i < SIZE; i++)
  {
    for (int j = 0; j < SIZE; j++)
    { 
         if (i < 8 && j < 8)
            lc.setLed (0, i, j, Matrix[i][j]);
         else  if (i < 8 && 8 <= j < 16)
            lc.setLed (1, i % 8, j % 8, Matrix[i][j]);
         else  if (8 <= i < 16 && j < 8)
            lc.setLed (2, i % 8, j % 8, Matrix[i][j]);
         else  if (8 <= i < 16 && 8 <= j < 16)
            lc.setLed (3, i % 8, j % 8, Matrix[i][j]);
    }
  }
}


void drawMatrix() {
  clearMatrix();

  //the row where the bottom of the player is at a given time
  int y = 15;

  if (validPoint(y, heroX)) {
    Matrix[y][heroX] = 1;
    Matrix[y - 1][heroX] = 1;
    Matrix[y - 2][heroX] = 1;
    Matrix[y - 3][heroX] = 1;
  }

  if (validPoint(y, heroX - 1)) {
    Matrix[y][heroX - 1] = 1;
    Matrix[y - 2][heroX - 1] = 1;
  }

  if (validPoint(y, heroX + 1)) {
    Matrix[y][heroX + 1] = 1;
    Matrix[y - 2][heroX + 1] = 1;
  }
   
}

//function for checking whether the point if between matrix boundaries or not
// y - row    x- column
boolean validPoint(int y, int x) {
  return  x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

void checkCollisions() {
  for (byte e = 0; e < 3; e++) {
    int ex = enemies[e].x;
    int ey = enemies[e].y;
    int diffX = abs(heroX - ex);

    if (ey >= 13 && diffX <= 2) {
      heroDie();
    }
  }
}


void heroDie() {
  deadTime = millis();
  playerAlive = false;
  displaySadFace();
  setupInitialVariables();
}

void checkActions() {
  if (digitalRead(JoyButton) ==  HIGH)
    pressed = true;
  if (pressed == true) {
    playerAlive = true;
  }
}

void displaySadFace() {
  for (int i = 0; i < SIZE; i++)
  {
    for (int j = 0; j < SIZE; j++)
    {  if (i < 8 && j < 8)
            lc.setLed (0, i, j, sadFace[i][j]);
         else  if (i < 8 && 8 <= j < 16)
            lc.setLed (1, i % 8, j % 8, sadFace[i][j]);
         else  if (8 <= i < 16 && j < 8)
            lc.setLed (2, i % 8, j % 8, sadFace[i][j]);
         else  if (8 <= i < 16 && 8 <= j < 16)
            lc.setLed (3, i % 8, j % 8, sadFace[i][j]);
    }
  }
}


// calibrate the joystick home for 10 times
void calibrateJoystick() {
  Coordinate values;

  for (int i = 0; i < 10; i++) {
    values.x += analogRead(JOY_X);
    values.y += analogRead(JOY_Y);
  }

  joystickHome.x = values.x / 10;
  joystickHome.y = values.y / 10;
}

void displayOnLcd() {
  TFTscreen.stroke(0, 0, 0);
  TFTscreen.rect(100, 0, 50, 50);
  TFTscreen.fill(0, 0, 0);
  
  char buff[20];
  itoa(score, buff, 10);
  char text[] = {'S','c','o','r','e',':','\0'};
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text(text, 0, 0);
  TFTscreen.text(buff, 100, 0);
}

void checkLevel() {
  if (score > 35) 
    vSpeed += 0.5;
}
