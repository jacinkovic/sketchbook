
#include <Adafruit_NeoPixel.h>

#define PIN 2   //D2

const int i_step = 2;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(120, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const uint32_t brightness_divider = 1; //3
uint32_t c_basetop, c_basemedium, c_basebody, c_basebottom, c_peak, c_empty;
int line;

const int analogReadsNum = 10;
const int topSize = 5;
const int mediumSize = 10;
const long inValDiv = 5;
const long peakSlow  = 1;  //10
const long baseSlow  = 10;  //1
long Lval, Rval;
long Lbase = 0, Rbase = 0;
long Lpeak = 0, Rpeak = 0;

const int analogInPinL = A7;
const int analogInPinR = A6;

void setup() {

  line = strip.numPixels() / 2;

  c_basetop = strip.Color( 255 / brightness_divider, 0, 0);
  //c_basetop = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_basebody = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_basebody = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_basebody = strip.Color( 0, 255 / brightness_divider, 0);
  //c_basebody = strip.Color( 255 / brightness_divider, 0, 0);
  //c_basebody = strip.Color( 0, 0, 255 / brightness_divider);
  //c_basetop = c_basebody;
  c_basemedium = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 0);
  //c_basebody = strip.Color( 0, 255 / brightness_divider, 0);
  c_basebottom = c_basebody;
  //c_basebottom = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 0);
  //c_peak = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_peak = strip.Color( 0, 0, 255 / brightness_divider);
  //c_peak = strip.Color( 255 / brightness_divider, 0 , 0);
  //c_peak = strip.Color( 255 / brightness_divider, 255 / brightness_divider , 0);
  //c_basetop = c_peak;
  c_empty = strip.Color( 0, 0, 0);
  //c_basetop = c_peak;
  strip.begin();
  strip.show();

  pinMode(analogInPinL, INPUT);
  pinMode(analogInPinR, INPUT);

  randomSeed(analogRead(8));
  //Serial.begin(115200);
}



void loop() {

  /*if( (millis() % 100) < 1){
    c_basebody = strip.Color( random(255) / brightness_divider, random(255) / brightness_divider, random(255) / brightness_divider);
    c_peak = strip.Color( random(255) / brightness_divider, random(255) / brightness_divider, random(255) / brightness_divider);
    c_basetop = c_peak;
    } */


  Lval = 0;
  Rval = 0;

  for (int i = 0; i < analogReadsNum; i++)
  {
    Lval = max(analogRead(analogInPinL), Lval);
    Rval = max(analogRead(analogInPinR), Rval);
  }

  Lval = Lval * 10; //no need for floating point, do it by integer
  Rval = Rval * 10;

  Lval = Lval / inValDiv;
  Rval = Rval / inValDiv;


  if (Lbase < Lval) {
    Lbase = Lval;
  }

  if (Rbase < Rval ) {
    Rbase = Rval;
  }

  if (Lpeak < Lbase ) {
    Lpeak = Lbase;
  }
  if (Rpeak < Rbase ) {
    Rpeak = Rbase;
  }

  vumeter(Lbase, Rbase, Lpeak, Rpeak);


  if (Lbase - baseSlow > 0) {
    Lbase = Lbase - baseSlow;
  }
  if (Rbase - baseSlow > 0) {
    Rbase = Rbase - baseSlow;
  }

  if (Lpeak - peakSlow > 0) {
    Lpeak = Lpeak - peakSlow;
  }
  if (Rpeak - peakSlow > 0) {
    Rpeak = Rpeak - peakSlow;
  }


}




void vumeter( int Lval, int Rval, int Lpeak, int Rpeak)
{
  int i;
  int tmppos;

  Lval = Lval / 10; //back to real analog value
  Rval = Rval / 10; //back to real analog value
  Lpeak = Lpeak / 10; //back to real analog value
  Rpeak = Rpeak / 10; //back to real analog value

  for (i = 0; i < line + line; i++) {
    strip.setPixelColor(i, c_empty);
  }

  for (i = line - Lval; i < line; i++) {
    strip.setPixelColor(i, c_basebody);
  }


  for (i = line; i < line + Rval; i++) {
    strip.setPixelColor(i, c_basebody);
  }

  for (i = 0; i < topSize; i++) {
    tmppos = line - Lval + i;
    if (tmppos < line) {
      strip.setPixelColor(tmppos, c_basetop);
    }
    tmppos = line + Rval - i - 1;
    if (tmppos >= line) {
      strip.setPixelColor(tmppos, c_basetop);
    }
  }

  for (i = topSize; i < mediumSize + topSize; i++) {
    tmppos = line - Lval + i;
    if (tmppos < line) {
      strip.setPixelColor(tmppos, c_basemedium);
    }
    tmppos = line + Rval - i - 1;
    if (tmppos >= line) {
      strip.setPixelColor(tmppos, c_basemedium);
    }
  }


  if (Lpeak > 0) {
    strip.setPixelColor(line - Lpeak - 1, c_peak);
  }

  if (Rpeak > 0) {
    strip.setPixelColor(line + Rpeak, c_peak);
  }

  strip.show();

}



