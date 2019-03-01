# Arcadino
Implementation of a mini-arcade which consists of five games made using Arduino Uno and four 8x8 LED matrix.

##Games Implemented

###Snake
The player has only one life and loses when the snake bites itself, but wins only when it has a length of 256 leds. Points are accumulated by eating the randomly spawned apples on the game board. 
Movement is controlled via joystick and the speed of the snake can be controlled with the potentiometer.

The game waits for input from the player (moving the joystick left or right) before starting. When the game ends, the final score is displayed and the same input is expected to reset everything and start from the beginning.

While playing, the score is displayed on an LCD.

###Car Race

The player has only one life and loses when he hits another car. Points are accumulated by avoiding the randomly generated cars on the game board. 
Movement is controlled via joystick. 

###Flappy Bird
The player has only one life and loses when he hits a tower or the bottom of the game board. Points are accumulated by avoiding the randomly generated tower on the game board. 
Movement is controlled via joystick. 

###Pong
The player has five lives or points and the computer has the same amount. 
Movement is controlled via joystick. 

###Stick Jump
The player jumps on platforms and has to jump on as many as possible. If he falls, you lose the game.
The plarforms dimension is decreasing for each level (max. three times).
Movement is controlled via joystick.

## Demo
* [A short video with the gameplay and some images]() 

## Hardware Components
* 3x Breadboard 

* 1x Arduino Mega

* 4x 8x8 LED matrix

* 4x Driver MAX7219

* 1x 1.8" LCD 

* 1x Joystick

* 4x 10k Ohm resistor

* 1x 220 Ohm resistor

* 4x 10μF Capacitor

* 4x 0.1μF Capacitor

* Bunch of wires


## Libraries

In addition to the source code you also need the LedControl.h Library installed.


