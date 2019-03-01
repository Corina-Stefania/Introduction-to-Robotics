#include "LedControl.h"
#include <TFT.h>
#include <SPI.h>

#define JOY_X A2
#define JOY_Y A3

#define CS_TFT  4
#define DC   24
#define RST  3

#define DIN 12
#define CLK 11
#define CS 10

LedControl lc = LedControl(DIN, CLK, CS, 4);
TFT TFTscreen = TFT(CS_TFT, DC, RST);

const int potentiometer = A5;
const int joyButton = 2;
const int bSpeed = 10;
const int default_time = 10;
const int playerSpeed = 2.5;
int potentiometer_value, x_val, y_val, button;

bool matrix[16][16] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

void lightMatrix();

struct Point
{
  int x, y;

  Point& operator= (const Point& other)
  {
    this->x = other.x;
    this->y = other.y;
    return *this;
  }
};

class Player
{
  private:
    Point m_position[3];

  public:
    void setPosition(Point, Point, Point);
    void moveRight();
    void moveLeft();
    Point showMidPosition();
};

Point Player::showMidPosition ()
{
  return m_position[1];
}

void Player::setPosition (Point left, Point middle, Point right)
{
  m_position[0] = left;
  m_position[1] = middle;
  m_position[2] = right;

  for (Point current : m_position)
  {
    matrix[current.x][current.y] = 1;
  }
}

void Player::moveLeft ()
{
  if (m_position[0].y > 0)
  {
    matrix[m_position[2].x][m_position[2].y] = 0;
    matrix[m_position[1].x][m_position[1].y] = 0;
    matrix[m_position[0].x][m_position[0].y] = 0;

    m_position[0].y = max((m_position[0].y - playerSpeed), 0);
    m_position[1].y = m_position[0].y + 1;
    m_position[2].y = m_position[1].y + 1;

    matrix[m_position[2].x][m_position[2].y] = 1;
    matrix[m_position[1].x][m_position[1].y] = 1;
    matrix[m_position[0].x][m_position[0].y] = 1;
  }
  lightMatrix();
}

void Player::moveRight ()
{
  if (m_position[2].y < 15)
  {
    matrix[m_position[2].x][m_position[2].y] = 0;
    matrix[m_position[1].x][m_position[1].y] = 0;
    matrix[m_position[0].x][m_position[0].y] = 0;

    m_position[2].y = min((m_position[2].y + playerSpeed), 15);
    m_position[1].y = m_position[2].y - 1;
    m_position[0].y = m_position[1].y - 1;

    matrix[m_position[2].x][m_position[2].y] = 1;
    matrix[m_position[1].x][m_position[1].y] = 1;
    matrix[m_position[0].x][m_position[0].y] = 1;
  }
  lightMatrix();
}

Player player, computer;
Point ball, next_ball;

int ball_x_dir, ball_y_dir, player_score = 0, computer_score = 0;

long long int current_time, start_time, current_ball_time, start_ball_time, pot_delay = 0;



void setup ()
{ lc.shutdown(0, false);
  lc.setIntensity(0, 2);
  lc.clearDisplay(0);

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
  TFTscreen.stroke(255, 255, 255);
  TFTscreen.text("Press button", 0, 10);
  TFTscreen.text("to START", 0, 50);

  drawPlayers();
  drawBall();

  start_ball_time = millis();
  start_time = millis();

  pinMode(joyButton, INPUT);

  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  showScore();
}

void loop ()
{
  if (player_score == 5)
  {
    playerWins();
  }
  else if (computer_score == 5)
  {
    computerWins();
  }
  else
  {
    current_time = millis();
    current_ball_time = millis();

    if ((current_time - start_time) > default_time)
    {
      movePlayer();
      moveComputer();

      start_time = current_time;
    }

    if ((current_ball_time - start_ball_time) > (default_time - pot_delay))
    {

      potentiometer_value = analogRead(potentiometer);
      if (potentiometer_value > 900 && pot_delay <= 200) pot_delay += bSpeed;
      else if (potentiometer_value < 200 && pot_delay >= 0) pot_delay -= bSpeed;

      moveBall();
      start_ball_time = current_ball_time;


    }
    lightMatrix();
  }
}

void drawPlayers ()
{
  for (int  i = 0; i < 16; i++)
  {
    matrix[0][i] = 0;
    matrix[15][i] = 0;
  }

  Point left_player, middle_player, right_player, left_computer, middle_computer, right_computer;
  left_player.x = 15;
  middle_player.x = 15;
  right_player.x = 15;

  left_player.y = 2;
  middle_player.y = 3;
  right_player.y = 4;

  left_computer.x = 0;
  middle_computer.x = 0;
  right_computer.x = 0;

  left_computer.y = 2;
  middle_computer.y = 3;
  right_computer.y = 4;
  player.setPosition(left_player, middle_player, right_player);
  computer.setPosition(left_computer, middle_computer, right_computer);
}

void lightMatrix ()
{
  for (int i = 0; i < 16; i++)
  {
    for (int j = 0; j < 16; j++)
    {
      if (i < 8 && j < 8)
        lc.setLed(0, i, j, matrix[i][j]);
      else  if (i < 8 && 8 <= j < 16)
        lc.setLed(1, i % 8, j % 8, matrix[i][j]);
      else  if (8 <= i < 16 && j < 8)
        lc.setLed(2, i % 8, j % 8, matrix[i][j]);
      else  if (8 <= i < 16 && 8 <= j < 16)
        lc.setLed(3, i % 8, j % 8, matrix[i][j]);
    }
  }
}

void movePlayer ()
{
  x_val = analogRead(JOY_X);

  if (x_val < 400) player.moveLeft();
  if (x_val > 650) player.moveRight();
}

void moveBall ()
{
  matrix[ball.x][ball.y] = false;
  next_ball.x = ball.x + ball_x_dir;
  next_ball.y = ball.y;

  if (matrix[next_ball.x][next_ball.y] == true)
  {
    ball_x_dir = -ball_x_dir;
    randomSeed(analogRead(0));
    int random_var = random(2, 13);
    if (random_var % 2) ball_y_dir = -ball_y_dir;

    if (ball.y + ball_y_dir > 15 || ball.y + ball_y_dir < 0)
      ball_y_dir = -ball_y_dir;

    ball.y += ball_y_dir;
    ball.x += ball_x_dir;
  }
  else
  {
    if (ball.y + ball_y_dir > 15 || ball.y + ball_y_dir < 0)
      ball_y_dir = -ball_y_dir;

    next_ball.y = ball.y + ball_y_dir;

    if (matrix[next_ball.x][next_ball.y] == true)
    {
      ball_x_dir = -ball_x_dir;

      int random_var = random(1, 11);
      if (random_var % 2) ball_y_dir = -ball_y_dir;

      if (ball.y + ball_y_dir > 15 || ball.y + ball_y_dir < 0)
        ball_y_dir = -ball_y_dir;

      ball.y += ball_y_dir;
      ball.x += ball_x_dir;
    }
    else if (next_ball.x == 0 && matrix[next_ball.x][next_ball.y] == false)
    {
      player_score++;
      drawBall();
      plusPoint();
    }
    else if (next_ball.x == 15 && matrix[next_ball.x][next_ball.y] == false)
    {
      computer_score++;
      drawBall();
      plusPoint();
    }
    else
    {
      ball.y += ball_y_dir;
      ball.x += ball_x_dir;
    }
  }
  matrix[ball.x][ball.y] = true;
}

void moveComputer ()
{
  Point computer_middle = computer.showMidPosition();

  Point next_ball = ball;
  next_ball.x += ball_x_dir;
  next_ball.y += ball_y_dir;

  if (next_ball.y > computer_middle.y) computer.moveLeft();
  if (next_ball.y < computer_middle.y) computer.moveRight();
}

void plusPoint ()
{
  showScore();
  lc.clearDisplay(0);
  lc.clearDisplay(1);
  lc.clearDisplay(2);
  lc.clearDisplay(3);

  for (int i = 0; i < 16; i++)
  {
    int j = 16 - i - 1;
    if (j < 8)
      lc.setLed(1, i % 8, j % 8, true);
    else  if (8 <= j < 16)
      lc.setLed(2, i % 8, j % 8, true);
    if (i < 8)
      lc.setLed(0, i, i, true);
    else  if (8 <= i < 16)
      lc.setLed(3, i % 8, i % 8, true);
  }
}

void playerWins ()
{
  TFTscreen.background(0, 0, 0);
  TFTscreen.text("You WIN :)", 0, 0);
  TFTscreen.text("Press Button", 0, 50);
  TFTscreen.text("to replay", 0, 90);
  while (1)
  {
    button = digitalRead(joyButton);
    if (button == HIGH)
      break;
  }
  player_score = 0;
  computer_score = 0;
  showScore();
}

void computerWins ()
{
  TFTscreen.background(0, 0, 0);
  TFTscreen.text("You Lose :(", 0, 0);
  TFTscreen.text("Press Button", 0, 50);
  TFTscreen.text("to replay", 0, 90);
  while (1)
  {
    button = digitalRead(joyButton);
    if (button == HIGH)
      break;
  }
  player_score = 0;
  computer_score = 0;
  showScore();
}

void drawBall ()
{
  drawPlayers();

  ball.x = 3;
  ball.y = 3;

  matrix[ball.x][ball.y] = true;
  randomSeed(analogRead(0));
  int random_y, random_x;
  random_y = random(10, 21);
  random_x = random(10, 21);

  if (random_y % 2) ball_y_dir = 1;
  else ball_y_dir = -1;

  if (random_x % 2) ball_x_dir = 1;
  else ball_x_dir = -1;
}

void showScore ()
{
  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);
  TFTscreen.text("SCORE", 3, 0);
  TFTscreen.text("Player: ", 3, 30);
  char buff[20], buff1[20];
  itoa(player_score, buff, 10);
  TFTscreen.text(buff, 95, 30);
  itoa(computer_score, buff1, 10);
  TFTscreen.text("Arcadino: ", 3, 60);
  TFTscreen.text(buff1, 110, 60);
}
