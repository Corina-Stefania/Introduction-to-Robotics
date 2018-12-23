# Stick Jump
Implementation of Doodle Jump like game using Arduino Uno and a 8x8 LED matrix.
The gameplay is simple but still entertaining because of the different platform size for each of its three levels.

## Demo
* [A short video with the gameplay and some images](https://drive.google.com/drive/folders/141rzbDIBwgn49zEEQjXYzLuxF50uEMZk?usp=sharing) 

## Hardware Components
* 4x Breadboard 

* 1x Arduino UNO

* 1x 8x8 LED matrix

* 1x Driver MAX7219

* 1x 16x2 LCD 

* 2x Buttons

* 2x 10k Ohm resistor

* 1x 100k Ohm resistor

* 1x 220 Ohm resistor

* 1x 10μF Capacitor

* 1x 0.1μF Capacitor

* Bunch of wires


### Circuit and schematics
![alt text](https://github.com/Corina-Stefania/Introduction-to-Robotics/blob/master/StickJump/Schematic/Stick_Jump_Circuit.png)

## Features
Two buttons:
* L: move player left.
* R: move player right.

Action: player jumps on platforms and has to jump on as many as possible. If he falls, you lose the game.

Decreasing platforms dimension for each level (max. three times).

Sad smiley for when you lose the game

All-Time-High-Score 


## Files

**config.h** -> Defines the Arduino pins used, start values of different enums in the code.

**StickJump.ino** -> this is the Arduino sketch that starts the program. It holds the loop function which controls the game according to the current game state.

## Libraries

In addition to the source code you also need the LedControl.h Library installed.

