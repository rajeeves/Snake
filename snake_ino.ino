#include <SoftReset.h>

#include <Wire.h>

#include <Adafruit_GFX.h>

#include <Adafruit_LEDBackpack.h>

#include <avr/wdt.h>

#include <LinkedList.h>

#include <iostream>

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define THRESHOLD 256
#define XPOT A0
#define YPOT A1
#define GETX(x) (x>>4)
#define GETY(x) (x & B1111)
#define DEBUG

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();
byte apple;

static const uint8_t PROGMEM
  frown_bmp[] =
  { B00111100,
    B01000010,
    B10100101,
    B10000001,
    B10011001,
    B10100101,
    B01000010,
    B00111100 };



template <typename T>
class SnakeList : public LinkedList<T>{
public:
  boolean search(T v){
    for (int i = 0; i < this->size(); ++i)
      if (this->get(i) == v)
        return true;
    return false;
  }
  void print(){
    for (int i = 0; i < this->size(); ++i, Serial.print(" ")){
      Serial.print("(");
      byte tmp = this->get(i);
      Serial.print(GETX(tmp));
      Serial.print(",");
      Serial.print(GETY(tmp));
      Serial.print(")");
    }
    Serial.print("\n");
  }
};

class Snake {
public:
  Snake() :
  dir(UP), list(){
    list.unshift(B01000100);
  }
  void makeMove(){
    byte loc = list.get(0);
    boolean outOfBounds = false;
    switch (dir){
    case UP:
      outOfBounds = GETY(loc) == 0;
      loc -= B1;
      break;
    case RIGHT:
      outOfBounds = GETX(loc) >= 7;
      loc += B10000;
      break;
    case DOWN:
      outOfBounds = GETY(loc) >= 7;
      loc += B1;
      break;
    case LEFT:
      outOfBounds = GETX(loc) == 0;
      loc -= B10000;
      break;
    default:
      #ifdef DEBUG
      Serial.print("Hey, ");
      Serial.print(dir);
      Serial.print(" isn't a direction!\n");
      #endif
      break;
    }
    #ifdef DEBUG
    Serial.print(GETX(loc));
    Serial.print(", ");
    Serial.print(GETY(loc));
    Serial.print("\n");
    #endif

    outOfBounds = outOfBounds || list.search(loc);
    list.unshift(loc);
    if (loc != apple){
      #ifdef DEBUG
      Serial.print("Apples and snakes successfully compared\n");
      list.print();
      #endif
      list.pop();
    }
    else {
      newApple();
    }

    if (outOfBounds){
      #ifdef DEBUG
      Serial.print("Oops!\n");
      #endif
      gameOver();
    }
    #ifdef DEBUG
    Serial.print("Move made\n");
    list.print();
    #endif
  }
  void changeDir(byte d){
    if (dir - d != 2 || d - dir != 2)
      dir = d;
  }
  byte getDir(){
    return dir;
  }
  SnakeList<byte>& getList(){
    return list;
  }
  void draw(){
    for (int i = 0; i < list.size(); ++i){
      byte temp = list.get(i);
      matrix.drawPixel(GETX(temp), GETY(temp), LED_ON);
      #ifdef DEBUG
      Serial.print(GETX(temp));
      Serial.print(", ");
      Serial.print(GETY(temp));
      Serial.print("\n");
      #endif
    }
  }
  ~Snake(){
  }
private:
  SnakeList<byte> list;
  byte dir;
} snake;

void setup(){
  matrix.begin(0x70);
  Serial.begin(9600);
  randomSeed(analogRead(3));
}

void loop(){
  matrix.clear();
  snake.draw();
  matrix.drawPixel(GETX(apple),GETY(apple),LED_ON);
  matrix.writeDisplay();
  unsigned long start = millis();
  byte newDir = snake.getDir();
  while (millis() - start < 1000){
    int x = analogRead(XPOT), y = analogRead(YPOT);

    if (x > 511 + THRESHOLD)
      newDir = RIGHT;
    else if (x < 511 - THRESHOLD)
      newDir = LEFT;
    else if (y > 511 + THRESHOLD)
      newDir = UP;
    else if (y < 511 - THRESHOLD)
      newDir = DOWN;

    if (snake.getDir() - newDir == 2 || newDir - snake.getDir() == 2) //force snake not to go back on itself
      newDir = snake.getDir();
  }
  snake.changeDir(newDir);
  snake.makeMove();
  #ifdef DEBUG
  Serial.print("\n\n");
  #endif
}

void newApple(){
  byte rand;
  do{
    rand = random(64);
    rand = (rand%8)<<4 + (rand/8);
  } while (snake.getList().search(rand));
  apple = rand;
}

void gameOver(){
  matrix.clear();
  matrix.drawBitmap(0,0,frown_bmp,8,8,LED_ON);
  matrix.writeDisplay();
  delay(3000);
  soft_restart();
}
