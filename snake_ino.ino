#include <SoftReset.h>

#include <Wire.h>

#include <Adafruit_GFX.h>

#include <Adafruit_LEDBackpack.h>

#include <avr/wdt.h>

#include <LinkedList.h>

#include <iostream>

#include <EEPROM.h>

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#define THRESHOLD 256
#define SELECT 2
#define XPOT A0
#define YPOT A1
#define GETX(x) (x>>4)
#define GETY(x) (x & B00001111)
#define DEBUG

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();
byte apple = 0;
volatile byte brightness = 4;
boolean pause = false;

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

void newApple(SnakeList<byte>& list){
  byte rand;
  do{
    rand = (byte) random(8);
    rand <<= 4;
    rand += (byte) random(8);
  } while (list.search(rand));
  apple = rand;
}

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
      newApple(this->list);
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
  newApple(snake.getList());
  matrix.setTextColor(LED_OFF,LED_ON);
  pinMode(SELECT, INPUT_PULLUP);
  attachInterrupt(0,flipPause,FALLING);
  //brightness = max(min(EEPROM.read(0),8),1);
  matrix.setBrightness(brightness);
}

void loop(){
  matrix.clear();
  if (pause){
    matrix.drawChar(0,0,brightness + 48,LED_ON,LED_OFF,1);
    matrix.writeDisplay();
    unsigned long start = millis();
    signed char newbright = 0;
    while (millis() - start < 500){
      int y = analogRead(YPOT);
      if (y > 511 + THRESHOLD)
        newbright = 1;
      else if ( y < 511 - THRESHOLD)
        newbright = -1;
    }
    if (newbright != 0){
      brightness += newbright;
      brightness = max(min(brightness,8),1);
      matrix.setBrightness(2*brightness-1);
    //  EEPROM.write(0,brightness);
    }
  }
  else{
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
}

void flipPause(){
  pause ^= true;
}


void gameOver(){
  matrix.clear();
  matrix.drawBitmap(0,0,frown_bmp,8,8,LED_ON);
  matrix.writeDisplay();
  delay(3000);
  soft_restart();
}
