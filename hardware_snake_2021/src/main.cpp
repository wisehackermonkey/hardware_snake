#include <Arduino.h>
#include "LedControl.h"
#include "Timer.h"
#include <SimpleList.h>
//---pins---
//joystick

//joystick pins
const int SW_PIN = 2; //digital pin connected to switch output
const int X_PIN = 1; // analog pin connected to X output
const int Y_PIN = 0; // analog pin connected to Y output

//joystick sensitivty
const int deadZone = 400;
const int x_upperbound = 1000 - deadZone;
const int x_lowerbound = deadZone;

const int y_upperbound = 1000 - deadZone;
const int y_lowerbound = deadZone;

//---8x8 max729 lcd grid setup
LedControl lc = LedControl(8, 10, 9, 1);


//--game player data
int x = 0;
int y = 0;
int x_vel = 0;
int y_vel = 0;

//used to blink player for more visalbity
bool isVisablePlayer = true;

//used to check wether player has moved
int x_prev = -1;
int y_prev = -1;

int x_joystick_raw = 0;
int y_joystick_raw = 0;

int x_joystick_dir = 0;
int y_joystick_dir = 0;

//constants
const int width = 8;
const int COIN_TOTAL = 3;



SimpleList<int> coins;

//---timer setup--
Timer t;
//event id number used by t.stop(ID_NAME)
int stopCoinSpawnEvent;
int coinEvent;
int playerBlinkEvent;
int moveEvent;

/*
  Game State Screens using bytes
  ---------------
  access buy using this
  formula ARRAY[width * y + x]

  --------------------
  get point x,y
  ----------
  ex point (1,1) = 4
  width = 8
  x = 1
  y = 1

  [width * y + x]
  [8 * 1 + 1]  = 4
  ----------------
  ex point (1,2) = 7
  width = 8
  x = 1
  y = 2

  [width * y + x]
  [8 * 2 + 1]  = 7

  NOTE:zero indexed starting at "0"

  -----> X
  | 0 1 0
  | 3 4 5
  | 6 7 9
  V
  Y
   0   1   2   3   4   5   6   7
  0  0,  1,  2,  3,  4,  5,  6,  7,
  1  8,  9, 10, 11, 12, 13, 14, 15,
  2  16, 17, 18, 19, 20, 21, 22, 23,
  3  24, 25, 26, 27, 28, 29, 30, 31,
  4  32, 33, 34, 35, 36, 37, 38, 39,
  5  40, 41, 42, 43, 44, 45, 46, 47,
  6  48, 49, 50, 51, 52, 53, 54, 55,
  7  56, 57, 58, 59, 60, 61, 62, 63

*/
byte screen[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};
byte playerScreen[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};

byte coinScreen[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};


void ClearScreen(byte []);
void flashPlayer(int , int , byte []);
void blinkPoint(int , int );
void Draw() ;
void EatCoin();
void StopCoinSpawning();
void AddCoin();
void ShowCoins();
void DisplayCoins();
void DisplayCharacter();
void MoveCharacter();
void ClearScreen(byte []);
void clearCoins();
void printList(SimpleList<int> );
void DoNothing();
void addPoint(int , int , byte []);
void clearPoint(int _x, int _y, byte Screen[]);
void ReadJoyStick();

void CalculateJoyStickDirection();


//helper function
void print(String str) {
  Serial.print(str);
}
void print(int str) {
  Serial.print(str);
}
void print(char str) {
  Serial.print(str);
}
void println(String str) {
  Serial.println(str);
}
void println(int str) {
  Serial.println(str);
}
void println(char str) {
  Serial.println(str);
}


void setup() {
  pinMode(SW_PIN, INPUT);
  digitalWrite(SW_PIN, HIGH);

  Serial.begin(9600);

  randomSeed(analogRead(A3));

  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  moveEvent = t.every(250, MoveCharacter, (void*)2);
  coinEvent = t.every(500, AddCoin, (void*)2);
  playerBlinkEvent = t.every(100, blinkPoint, (void*)2);
  stopCoinSpawnEvent = t.after(5000, StopCoinSpawning, (void*)2);

  coins.reserve(300);
}

void loop() {
  t.update();
  ReadJoyStick();
  CalculateJoyStickDirection();
  EatCoin();
  DisplayCoins();
  DisplayCharacter();
  Draw();
}

void DisplayCharacter() {
  ClearScreen(playerScreen);
  //  addPoint(x, y, playerScreen);
  flashPlayer(x, y, playerScreen);
}

void DisplayCoins() {
  ClearScreen(coinScreen);
  ShowCoins();
}
void ShowCoins() {
  for (SimpleList<int>::iterator itr = coins.begin(); itr != coins.end(); ++itr) {
    int _coinLoc = (*itr);
    int _x = _coinLoc % width;
    int _y = (_coinLoc - (_coinLoc % width)) / width;
    addPoint(_x, _y, coinScreen);
  }
}

void AddCoin() {
  //prevent filling up ram
  //  if (coins.size() <= COIN_TOTAL) {
  int _x = random(0, 8);
  int _y = random(0, 8);
  coins.push_back(width * _y + _x);

  //  }
}

void StopCoinSpawning() {
  t.stop(coinEvent);
}

void EatCoin() {

  int playerPosition = width * y + x;
  for (SimpleList<int>::iterator itr = coins.begin(); itr != coins.end(); ) {
    if ((*itr) == playerPosition) {

      int _x = (*itr) % width;
      int _y = ((*itr) - ((*itr) % width)) / width;
      clearPoint(_y, _x, coinScreen);
      clearPoint(_y, _x, playerScreen);
      itr = coins.erase(itr);
      //      Draw();
      continue;
    }
    ++itr;
  }
}

void Draw() {
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, screen[i] | playerScreen[i] | coinScreen[i]);
  }
}


void blinkPoint(int _x, int _y) {
  isVisablePlayer = ! isVisablePlayer;

}
void flashPlayer(int _x, int _y, byte Screen[]) {
  Screen[_y] = (isVisablePlayer == true) ? bitSet(Screen[_y], _x) : bitClear(Screen[_y], _x);
}

void ClearScreen(byte grid[]) {
  for (int i = 0; i < 8; i++) {
    grid[i] = B00000000;
  }
}


void MoveCharacter() {
  x += x_joystick_dir;
  y += y_joystick_dir;
}



void clearCoins() {
  for (SimpleList<int>::iterator itr = coins.begin(); itr != coins.end(); ++itr) {
    int _coinLoc = (*itr);
    int _x = _coinLoc % width;
    int _y = (_coinLoc - (_coinLoc % width)) / width;
    clearPoint(_x, _y, coinScreen);
  }
}





void printList(SimpleList<int> arr) {
  for (SimpleList<int>::iterator itr = arr.begin(); itr != arr.end(); ++itr) {
    Serial.print((*itr));
    Serial.print(",");
  }
  println("");
}


void DoNothing() {
}


void addPoint(int _x, int _y, byte Screen[]) {
  Screen[_y] = bitSet(Screen[_y], _x);
}
void clearPoint(int _x, int _y, byte Screen[]) {
  Screen[_y] = bitClear(Screen[_y], _x);

}

void ReadJoyStick() {
  x_joystick_raw = analogRead(X_PIN);
  y_joystick_raw = analogRead(Y_PIN);
}

void CalculateJoyStickDirection() {
  if ( x_upperbound < x_joystick_raw) {
    x_joystick_dir = 1;
  } else if ( x_lowerbound > x_joystick_raw) {
    x_joystick_dir = -1;
  } else {
    x_joystick_dir = 0;
  }

  if ( y_upperbound < y_joystick_raw) {
    y_joystick_dir = 1;
  } else if ( y_lowerbound > y_joystick_raw) {
    y_joystick_dir = -1;
  } else {
    y_joystick_dir = 0;
  }
}

