#include "config.h"
#include <EEPROM.h>
#include "LedControl.h"       //need the library
#include <TFT.h>
#include <SPI.h>

LedControl lc = LedControl(DIN, CLK, LOAD, 4);
TFT TFTscreen = TFT(cs, dc, rst);


//*--------------Variables ------------------*//


//For lcd and Matrix refresh
unsigned long lastUpdate = 0;
unsigned long lastLcdUpdate = 0;

//For score and levels
int score;
float allTimeHighScore = 0.00f;
int level;
//EEPROM address to start reading from for allTimeHighScore
int eeAddress = 0;


// Player
typedef struct {
  int col;
  int row;

  float velocityY;
} Player;

Player player;

//Player movement
const float gravity = -0.4;
const float jumpVelocity = 3.5;
const float gameSpeed = 0.5;
// the maximum height before screen scroll
const int playerMaximumOnScreenY = 10;

boolean playerAlive;



// the matrix state
bool Matrix[SIZE][SIZE] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

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


// the currentscreen offset
int screenOffset;


//Platforms
typedef enum {
  ACTIVE, INACTIVE
} PlatformState;

typedef struct {
  int x;
  int y;

  PlatformState state;
} Platform;

//one platform per matrix row
Platform platforms[SIZE];

// the index of the platform from lowest row
int platformSmallestYIndex;

int lastActivePlatformIndex;


//*------------ Arduino ----------------*//

void setup()
{
  // the zero refers to the MAX7219 number, it is zero for 1 chip
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

  //Get the float data from the EEPROM at position 'eeAddress'
  EEPROM.get( eeAddress, allTimeHighScore );

  TFTscreen.begin();

  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);

  //set the text size
  TFTscreen.setTextSize(2);
  TFTscreen.setRotation(3);
  
  // set the text color to white
  TFTscreen.stroke(255, 255, 255);
  
  char buff[100];
  itoa(allTimeHighScore, buff, 10);
  TFTscreen.text("HighScore: ", 0, 0);
  TFTscreen.text(buff, 20, 0);
  TFTscreen.text("< or > to start", 0, 10);

  clearMatrix();
  setupInitialVariables();
  drawMatrix();
}

void loop() {
  switch (state) {
    case SETUP:
      checkActions();
      break;

    case LOST:
      playerAlive = false;
      setupInitialVariables();
      drawMatrix();
      state = SETUP;
      break;

    case PLAY:
      int refreshInterval = 1;
      if ((millis() - lastUpdate) >= refreshInterval) {
        lastUpdate = millis();
        if (playerAlive) {
          updatePlayerColumn();
          updatePlayerRow();
          displayOnLcd();
          drawMatrix();
          checkLevel();
          displayMatrix();
          checkForDeath();
        } else {
          checkActions();
        }
      }
  }
}

//*---------------------------------------------------------*//





//*-------- Setup Functions -----------*//

void setupInitialVariables() {
  platformSmallestYIndex = 0;
  player.col = SIZE / 2 - 1;
  player.row = 8;
  player.velocityY = 0;

  screenOffset = 0;

  for (int i = 0; i < SIZE; i++) {
    Platform p;
    p.x = random(SIZE - 1); //col
    p.y = i; //row
    p.state = INACTIVE;
    platforms[i] = p;
  }

  platforms[0].x = SIZE / 2 - 2;
  platforms[0].state = ACTIVE;

  platforms[3].x = 8;
  platforms[3].state = ACTIVE;

  platforms[5].x = 0;
  platforms[5].state = ACTIVE;

  platforms[7].x = 0;
  platforms[7].state = ACTIVE;

  lastActivePlatformIndex = 7;

  platformLevel = LEVEL1;
}

//*---------------------------------------------------------*//




//*--------- Matrix state functions --------------*//

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

  //the row where the player is at given time
  int y = actualYPosition(player.row);

  if (validPoint(player.col, y)) {
    Matrix[y][player.col] = 1;
  }

  if (validPoint(player.col, y + 1)) {
    Matrix[y + 1][player.col] = 1;
  }
}

//player.row or the player "height" can go up much higher than the matrix size which is 16
//we use this function for checking where he actually is on the matrix
int actualYPosition (int y) {
  return SIZE - 1 - (y - screenOffset);
}

//function for checking whether the point if between matrix boundaries or not
boolean validPoint(int x, int y) {
  return  x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}


void displaySadFace() {
  for (int i = 0; i < SIZE; i++)
  {
    for (int j = 0; j < SIZE; j++)
    { if (i < 8 && j < 8)
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

//*---------------------------------------------------------*//






//*-------- Player movement --------*//

//left and right movement
void updatePlayerColumn() {
  //we use the same principle from modulo numbers so that
  //when the player moves left or right and exceeds the matrix boundaries
  //he can reappear/come back from the opposite side
  if (analogRead(JOY_X) < 400) {          //left
    player.col = (player.col + SIZE - 1) % SIZE;
  }
  if (analogRead(JOY_X) > 650) {        //right
    player.col = (player.col + 1) % SIZE;
  }
}


//up and down movement
void updatePlayerRow() {
  //player goes up and down
  player.velocityY += gravity;
  int y = player.velocityY + gameSpeed * player.velocityY;
  if ( actualYPosition(y) < 16)
    player.row += y;
  else player.row += y - 1;
  score = player.row / 2;

  //if the player is falling and has a platform underneath
  //we can restore its velocity so it can bounce again
  if (player.velocityY <= 0 && playerHasPlatform()) {
    player.velocityY = jumpVelocity;
  }

  //if we need to move the screen (scroll)
  if (player.row - screenOffset > playerMaximumOnScreenY) {
    screenOffset++;
    updatePlatforms();
  }
}


boolean playerHasPlatform() {
  for (int i = 0; i < SIZE; i++) {
    Platform p = platforms[i];
    if (p.state == ACTIVE && platformLevel == LEVEL1 &&
        p.y - screenOffset >= 0 &&
        (player.row == p.y + 2 || player.row  == p.y || player.row == p.y + 1) &&
        (player.col == p.x || player.col == p.x + 1 || player.col == p.x + 2)) {
      return true;
    }

    if (p.state == ACTIVE && platformLevel == LEVEL2 &&
        p.y - screenOffset >= 0 &&
        (player.row == p.y || player.row == p.y + 1) &&
        (player.col == p.x || player.col == p.x + 1)) {
      return true;
    }

    if (p.state == ACTIVE && platformLevel == LEVEL3 &&
        p.y - screenOffset >= 0 &&
        player.row == p.y + 1 &&
        player.col == p.x ) {
      return true;
    }
  }
  return false;
}


void checkForDeath() {
  if (player.row < screenOffset) {
    if (score > allTimeHighScore) {
      allTimeHighScore = score;
      EEPROM.put( eeAddress, allTimeHighScore);
    }
    state = LOST;
    playerAlive = false;
    displaySadFace();
    displayGameLostMessage();
  }
}

//*---------------------------------------------------------*//






//* ---------------- Platforms ------------*//

void drawPlatform(int x) {
  for (int i = 0; i < SIZE; i++) {
    Platform p =  platforms[i];
    int y =  actualYPosition(p.y);

    if (p.state == ACTIVE) {
      for (int j = 0; j < x; j++) {
        if (validPoint(p.x + j, y)) {
          Matrix[y][p.x + j] = 1;
        }
      }
    }
  }
}


void updatePlatforms() {
  if (platforms[platformSmallestYIndex].y < screenOffset) {
    platforms[platformSmallestYIndex].y += SIZE;

    if (platforms[platformSmallestYIndex].y - lastActivePlatformIndex > 8
        || (platforms[platformSmallestYIndex].y - lastActivePlatformIndex > 4 && random(4) == 0)) {
      platforms[platformSmallestYIndex].x = random(SIZE - 1);
      platforms[platformSmallestYIndex].state = ACTIVE;
      lastActivePlatformIndex = platforms[platformSmallestYIndex].y;
    } else {
      platforms[platformSmallestYIndex].state = INACTIVE;
    }
    platformSmallestYIndex = (platformSmallestYIndex + 1) % SIZE;
  }
}

//*---------------------------------------------------------*//



//* -------- Game logic and game state checking ----------*//

void checkActions() {
  if (analogRead(JOY_X) < 400 || analogRead(JOY_X) > 600) {
    playerAlive = true;
    state = PLAY;
  }
}


void checkLevel() {
  int x = 0;
  switch (platformLevel) {
    case LEVEL1:
      x = 3;      //platform ...
      drawPlatform(x);
      if (player.row > 100) {
        platformLevel = LEVEL2;
      }
      break;

    case LEVEL2:
      x = 2;       //platform ..
      drawPlatform(x);

      if (player.row > 250) {
        platformLevel = LEVEL3;
      }
      break;

    case LEVEL3:
      x = 1;      //platform .
      drawPlatform(x);
      break;
  }
}

//*---------------------------------------------------------*//







//*---------------- Display --------------------------------*//

void displayOnLcd() {
  int Recalculating = 100;
  if ((millis() - lastLcdUpdate) >= Recalculating) {
    // clear the screen with a black background
    TFTscreen.background(0, 0, 0);
    
    lastLcdUpdate = millis();
    if (player.row > 0) {
      TFTscreen.text("Scor: ", 6, 1);
      char buff[20];
      itoa(score, buff, 10);
      TFTscreen.text(buff, 65, 1);
    } else {
      TFTscreen.text("Scor: 0", 6, 1);
    }

    if (platformLevel == LEVEL1) {
      TFTscreen.text("Level: 1", 6, 50);
    } else if (platformLevel == LEVEL2) {
      TFTscreen.text("Level: 2", 6, 50);
    } else {
      TFTscreen.text("Level: 3", 6, 50);
    }
  }
}

void displayGameLostMessage() {
  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);

  if (score > allTimeHighScore) {
    allTimeHighScore = score;
    TFTscreen.text("New HighScore: ", 6, 57);
    char buff[100];
    itoa(allTimeHighScore, buff, 10);
    TFTscreen.text(buff, 75, 57);
  } else {
    TFTscreen.text("Score: \n", 6, 57);
    TFTscreen.text(score, 70, 57);
  }
  TFTscreen.text("< or > to start", 6, 87);
}

//*---------------------------------------------------------*//
