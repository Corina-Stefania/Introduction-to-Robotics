#include "config.h"
#include <EEPROM.h>
#include "LedControl.h"       //need the library
#include <LiquidCrystal.h>
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);

// pin 12 is connected to the MAX7219 pin 1
// pin 11 is connected to the CLK pin 13
// pin 10 is connected to LOAD pin 12
// 1 as we are only using 1 MAX7219
LedControl lc = LedControl(12, 11, 10, 1); //DIN, CLK, LOAD, No. DRIVER



//*--------------Variables ------------------*//


//For lcd and Matrix refresh
unsigned long lastUpdate = 0;
unsigned long lastLcdUpdate = 0;



//For score and levels
int score;
float allTimeHighScore = 0.00f;
int level;
int eeAddress = 0; //EEPROM address to start reading from for allTimeHighScore


// Player
typedef struct {
  int col;
  int row;

  float velocityY;
} Player;

Player player;

//Player movement
const float gravity = -0.4;
const float jumpVelocity = 3;
const int playerMaximumOnScreenY = 5;  // the maximum height before screen scroll

// death
boolean playerAlive;
//long timeOfDeath;


// the matrix state
bool Matrix[SIZE][SIZE] = {
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0}
};

//game lost display
bool sadFace[8][8] {
  { 1, 1, 1, 1, 1, 1, 1, 1},
  { 1, 0, 0, 0, 0, 0, 0, 1},
  { 1, 0, 1, 0, 0, 1, 0, 1},
  { 1, 0, 0, 0, 1, 0, 0, 1},
  { 1, 0, 0, 0, 1, 0, 0, 1},
  { 1, 0, 1, 0, 0, 1, 0, 1},
  { 1, 0, 0, 0, 0, 0, 0, 1},
  { 1, 1, 1, 1, 1, 1, 1, 1}
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

  //Get the float data from the EEPROM at position 'eeAddress'
  EEPROM.get( eeAddress, allTimeHighScore );

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HighScore: " + String((int)allTimeHighScore));
  lcd.setCursor(0, 1);
  lcd.print("< or > to start");
  setupPins();
  clearMatrix();
  setupInitialVariables();
  drawMatrix();

  //    for (int i = 0 ; i < EEPROM.length() ; i++) {
  //      EEPROM.write(i, 0);
  //    }

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
      int refreshInterval = 75;
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

void setupPins() {
  pinMode(BUTTON_RIGHT_PIN, INPUT);
  pinMode(BUTTON_LEFT_PIN, INPUT);
  pinMode(V0_PIN, OUTPUT);
  analogWrite(V0_PIN, 120);
}

void setupInitialVariables() {
  platformSmallestYIndex = 0;
  player.col = SIZE / 2 - 1;
  player.row = 4;
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

  platforms[3].x = 5;
  platforms[3].state = ACTIVE;

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
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      lc.setLed(0, i, j, Matrix[i][j]);
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

//player.row or the player "height" can go up much higher than the matrix size which is 8
//we use this function for checking where he actually is on the matrix
int actualYPosition (int y) {
  return SIZE - 1 - (y - screenOffset);
}

//function for checking whether the point if between matrix boundaries or not
boolean validPoint(int x, int y) {
  return  x >= 0 && x < SIZE && y >= 0 && y < SIZE;
}


void displaySadFace() {
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      lc.setLed(0, i, j, sadFace[j][i]);
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
  if (digitalRead(BUTTON_LEFT_PIN) == HIGH) {
    player.col = (player.col + SIZE - 1) % SIZE;
  }
  if (digitalRead(BUTTON_RIGHT_PIN) == HIGH) {
    player.col = (player.col + 1) % SIZE;
  }
}


//up and down movement
void updatePlayerRow() {
  //player goes up and down
  player.velocityY += gravity;
  player.row += player.velocityY;
  score = player.row / 5;

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
        (player.row == p.y || player.row == p.y + 1) &&
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
        player.row == p.y &&
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

    if (platforms[platformSmallestYIndex].y - lastActivePlatformIndex > 4
        || (platforms[platformSmallestYIndex].y - lastActivePlatformIndex > 2 && random(3) == 0)) {
      // 5th inactive in a row OR ( 33% chance AND not two inactives before it )
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
  if (digitalRead(BUTTON_LEFT_PIN) == HIGH || digitalRead(BUTTON_RIGHT_PIN) == HIGH) {
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
      if (player.row > 50) {
        platformLevel = LEVEL2;
      }
      break;

    case LEVEL2:
      x = 2;       //platform ..
      drawPlatform(x);

      if (player.row > 150) {
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
    lastLcdUpdate = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    if (player.row > 0) {
      lcd.print("Scor: " + String((int)score));
    } else {
      lcd.print("Scor: 0");
    }

    lcd.setCursor(0, 1);
    if (platformLevel == LEVEL1) {
      lcd.print("Level: 1");
    } else if (platformLevel == LEVEL2) {
      lcd.print("Level: 2");
    } else {
      lcd.print("Level: 3");
    }
  }
}

void displayGameLostMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (score > allTimeHighScore) {
    allTimeHighScore = score;
    lcd.print("New HighScore: " + String(allTimeHighScore));
  } else {
    lcd.print("Score: " + String((int)score));
  }
  lcd.setCursor(0, 1);
  lcd.print("< or > to start");
}

//*---------------------------------------------------------*//
