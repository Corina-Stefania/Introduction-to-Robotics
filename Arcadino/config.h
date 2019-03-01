#define SIZE 16
#define JOY_X A2 
#define JOY_SW 2

#define CS   4
#define DC   24
#define RST  3

#define DIN 12
#define CLK 11
#define LOAD 10


enum {SETUP, PLAY, LOST} state = PLAY;
enum {LEVEL1, LEVEL2, LEVEL3} platformLevel = LEVEL1;
