#include "LedControl.h"
#include <TFT.h>  
#include <SPI.h>

// default pins
#define JOY_X A2
#define JOY_Y A3
#define SIZE 16
#define JoyButton 2

#define CS   4
#define DC   24
#define RST  3

#define DIN 12
#define CLK 11
#define LOAD 10 //CS

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

float heroY = 5;
float vSpeed = 1;
bool playerAlive;

//position of the tower to be avoided
float towerX = 15; 

float floorOffset = 1;
//speed of the scene (we move the towers to the left, where the bird is)
float hSpeed = 1; 

//position of the gap in the tower
int gapY = 3; 
 
int lastUpdate = 0;
int score = 0;

unsigned long deadTime = millis(); 

struct Coordinate {
  int x = 0, y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};

// threshold where movement of the joystick will be accepted
const int joystickThreshold = 160;
// construct with default values in case the user turns off the calibration
Coordinate joystickHome(500, 500);

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

void setup(){
    pinMode(JoyButton, INPUT);

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


  TFTscreen.begin();
  
 // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  
  //set the text size
  TFTscreen.setTextSize(2);
  TFTscreen.setRotation(3); 
  
  // set the text color to white
  TFTscreen.stroke(255,255,255);

  clearMatrix();
  calibrateJoystick();
  drawMatrix();
}

void loop() {
  int refreshInterval = 25;
      if ((millis() - lastUpdate) >= refreshInterval) {
        lastUpdate = millis();
        if (playerAlive) {
            updatePlayerRow();
          displayOnLcd();
          drawMatrix();
          drawTower();
          displayMatrix();
          checkCollisions();
        } else {
          checkActions();
        }
      }
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
            lc.setLed (1, i%8, j%8, Matrix[i][j]);
         else  if (8 <= i < 16 && j < 8)
            lc.setLed (2, i%8, j%8, Matrix[i][j]);
         else  if (8 <= i < 16 && 8 <= j < 16)
            lc.setLed (3, i%8, j%8, Matrix[i][j]);
    }
  }
}


void drawMatrix() {
  clearMatrix();
  /*
        hero looks like:

            *
           ***  <- the center * is the Y position
            * 
     */

   //column
    int x = 2;
    
  if (validPoint(heroY, x)) {
    Matrix[(int)heroY][x] = 1;
    Matrix[(int)heroY][x-1] = 1;
    Matrix[(int)heroY][x + 1] = 1;
  }

  if (validPoint(heroY + 1, x)) {
    Matrix[(int)heroY + 1][x] = 1;
  }

  if (validPoint(heroY - 1, x)) {
    Matrix[(int)heroY - 1][x] = 1;
  }
}

boolean validPoint(int y, int x) {
  return  x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}

void updatePlayerRow() {
    int buttonState = analogRead(JOY_Y);
    //move hero
    heroY += vSpeed; 

    //if hero touches the bottom he dies
    if(heroY >= 15) 
        heroDie();

    //move the screen offset
    floorOffset += hSpeed; 
    if(floorOffset > 4)
        floorOffset = 0;

    if (buttonState < 400) {
      jump();
    }
}

void drawTower(){
  //move the tower to the left
    towerX -= hSpeed; 
    score ++;
    int x = (int)towerX;
    // if moved to 0, returns to the right
    if(x < 0){ 
        x = 15;
        towerX = 15;
        gapY = random(0, 8); //position of the gap is randomly generated
    }

    //draw the tower
    for(int i = 0; i < 16; i++){ 
        if(i <= gapY || i >= gapY + 7){
            Matrix[i][x] = 1;
            Matrix[i][x+1] = 1;
        }
    }

    if(x == 2)
        checkCollisions();
}


void checkCollisions(){ 
    if(heroY < gapY || heroY > gapY + 7) {
      heroDie();
    }      
}

void heroDie(){
     playerAlive = false;
    displaySadFace();
    restartGame();
    deadTime = millis();
    checkActions();
}

void restartGame(){
    heroY = 5;
    towerX = 15;
    floorOffset = 1;
    gapY = 3;
    score = 0;
    TFTscreen.background(0, 0, 0);
}

void jump(){
    if(heroY > 0)
        heroY -= 2;
}

void checkActions() {
  if (digitalRead(JoyButton) ==  HIGH)
    playerAlive = true;
}

void displaySadFace() {
  for (int i = 0; i < SIZE; i++)
  {
    for (int j = 0; j < SIZE; j++)
    {  if (i < 8 && j < 8)
            lc.setLed (0, i, j, sadFace[i][j]);
         else  if (i < 8 && 8 <= j < 16)
            lc.setLed (1, i%8, j%8, sadFace[i][j]);
         else  if (8 <= i < 16 && j < 8)
            lc.setLed (2, i%8, j%8, sadFace[i][j]);
         else  if (8 <= i < 16 && 8 <= j < 16)
            lc.setLed (3, i%8, j%8, sadFace[i][j]);
    }
  }
}

void displayOnLcd() {
  TFTscreen.stroke(0, 0, 0);
  TFTscreen.rect(90, 0, 50, 50);
  TFTscreen.fill(0, 0, 0);
  
  char buff[20];
  itoa (score, buff, 10);
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("Score: ", 0, 0);
  TFTscreen.text(buff, 90, 0);
}
