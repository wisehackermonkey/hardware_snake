// Simplistic hardware snake game using joystick and 8x8 led display
// by oran collins
// github.com/wisehackermonkey
// oranbusiness@gmail.com
// 20210110

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
int score = 0;

//used to blink player for more visalbity
bool isVisablePlayer = true;

//used to check wether player has moved
int x_prev = -1;
int y_prev = -1;

int x_joystick_raw = 0;
int y_joystick_raw = 0;

int x_joystick_dir = 0;
int y_joystick_dir = 0;
bool joystick_button = false;

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
const byte X_ENDGAME_SCREEN[8] = {
  B10000001,
  B01000010,
  B00100100,
  B00011000,
  B00011000,
  B00100100,
  B01000010,
  B10000001
};

const byte WIN_SCREEN[8] = {
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111,
  B11111111
};
const byte WIN_SCREEN_INVERTED[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000,
  B00000000
};




const byte X_ENDGAME_SCREEN_INTERTED[8] = {
  B01111110,
  B10111101,
  B11011011,
  B11100111,
  B11100111,
  B11011011,
  B10111101,
  B01111110
};


void ClearScreen(byte []);
void flashPlayer(int , int , byte []);
void blinkPoint(int , int );
void Draw() ;
void Draw(byte []);
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
void clearPoint(int , int , byte []);
void ReadJoyStick();
void ScreenWrap() ;
void WallCollision();
void CalculateJoyStickDirection();
void FillScreen(byte []);
void DespawnCoin();

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
  playerBlinkEvent = t.every(75, blinkPoint, (void*)2);
  stopCoinSpawnEvent = t.after(2000, StopCoinSpawning, (void*)2);

  int despawnCoinEvent = t.every(6000, DespawnCoin, (void*)2);
  coins.reserve(300);
}

bool check_win = false;
void loop() {
  t.update();
  if (score <= 5 && coins.size() >= 1) {
    ReadJoyStick();
    CalculateJoyStickDirection();
    EatCoin();
    if(check_win){
      if(score == COIN_TOTAL){
        Serial.println("WIN!");

  delay(250);

        ClearScreen(coinScreen);
    Draw();
                

                   delay(250);
    Draw(WIN_SCREEN);
     delay(250);
    Draw(WIN_SCREEN_INVERTED);
    delay(250);

      }
    }
    DisplayCoins();
    DisplayCharacter();
    Draw();
  } else if(score < 0 && coins.size() == 0){
    FillScreen(screen);
    Draw();
    delay(250);

        ClearScreen(coinScreen);
    Draw();

    delay(250);
    Draw(X_ENDGAME_SCREEN);
     delay(250);
    Draw(X_ENDGAME_SCREEN_INTERTED);
    delay(250);
  }



}

void FillScreen(byte Screen[]) {
  for (int i = 0; i < 8; i++) {
    Screen[i] = B11111111;
  }
}
void WallCollision() {


  if (x <= -1) {
    x = 0;
  } else if (x > 7) {
    x = 7;
  }

  if (y <= -1) {
    y = 0;
  } else if (y > 7) {
    y = 7;
  }
}

void ScreenWrap() {

  if (x <= -1) {
    x = 7;
  } else if (x > 7) {
    x = 0;
  }

  if (y <= -1) {
    y = 7;
  } else if (y > 7) {
    y = 0;
  }
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
  //  if (coins.size() <= COIN_TOTAL) {}
  int _x = random(0, 8);
  int _y = random(0, 8);
  coins.push_back(width * _y + _x);
  
}
void DespawnCoin() {
  coins.pop_front();
  score--;
}
void StopCoinSpawning() {
  t.stop(coinEvent);
  check_win = true;
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


      //Progress in game
      //Register eating a coin as incrementing score
      score++;
      Draw();
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
void Draw(byte Screen[]) {
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, Screen[i]);
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
  WallCollision();
  //ScreenWrap();
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
  Serial.println("");
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
void(* resetFunc) (void) = 0;

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

  if (joystick_button == HIGH) {
    //todo reset location on button click
    resetFunc(); 
  }
}

