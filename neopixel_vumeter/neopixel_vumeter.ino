
#include <Adafruit_NeoPixel.h>

#define PIN 3

const int i_step = 2;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(120, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const uint32_t brightness_divider = 1; //3
uint32_t c_basetop, c_basebody, c_basebottom, c_peak, c_empty;
int line;

const int inValDiv = 2;
const int peakSlow  = 10;  //10
const int baseSlow  = 1;  //1
int Lval, Rval;
int Lbase = 0, Rbase = 0;
int Lpeak = 0, Rpeak = 0;

const int analogInPinL = A1;
const int analogInPinR = A0;

void setup() {
  
  line = strip.numPixels() / 2;

  //c_basetop = strip.Color( 255 / brightness_divider, 0, 0);
  //c_basebody = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_basebody = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_basebody = strip.Color( 0, 255 / brightness_divider, 0);
  c_basebody = strip.Color( 255 / brightness_divider, 0, 0);
  //c_basebody = strip.Color( 0, 0, 255 / brightness_divider);
  c_basetop = c_basebody;
  //c_basebody = strip.Color( 0, 255 / brightness_divider, 0);
  c_basebottom = c_basebody;
  //c_basebottom = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 0);
  c_peak = strip.Color( 255 / brightness_divider, 255 / brightness_divider, 255 / brightness_divider);
  c_peak = strip.Color( 255 / brightness_divider, 0 , 0);
  //c_peak = strip.Color( 255 / brightness_divider, 255 / brightness_divider ,0);
  //c_basetop = c_peak;
  c_empty = strip.Color( 0, 0, 0);

  strip.begin();
  strip.show();

  pinMode(analogInPinL, INPUT);
  pinMode(analogInPinR, INPUT);

  //Serial.begin(115200);
}



void loop() {
  for (int i = 0; i < 10; i++)
  {
    Lval = max(analogRead(analogInPinL), Lval);
    Rval = max(analogRead(analogInPinR), Rval);;
  }

  Lval = Lval / inValDiv;
  Rval = Rval / inValDiv;

  if (Lval > line - 1) {
    Lval = line - 1 ;
  }
  if (Rval > line - 1) {
    Rval = line - 1 ;
  }

  //Lbase = Lval;
  //Rbase = Rval;
  //    Serial.println(Lval);

  if (Lval * baseSlow  > Lbase) {
    Lbase = (Lval + 1) * baseSlow - 1;
  }
  //Lbase = Lval * baseSlow;

  if (Rval * baseSlow  > Rbase) {
    Rbase = (Rval + 1) * baseSlow - 1;
  }

  Lval = Lbase / baseSlow;
  Rval = Rbase / baseSlow;

  if (Lval * peakSlow  > Lpeak) {
    Lpeak = (Lval + 1) * peakSlow - 1;
  }
  if (Rval * peakSlow  > Rpeak) {
    Rpeak = (Rval + 1) * peakSlow - 1;
  }

  vumeter(Lbase / baseSlow, Rbase / baseSlow, Lpeak / peakSlow , Rpeak / peakSlow);

  Lpeak--;
  Rpeak--;
  if (Lpeak < 0) {
    Lpeak = 0;
  }
  if (Rpeak < 0) {
    Rpeak = 0;
  }

  Lbase--;
  Rbase--;
  if (Lbase < 0) {
    Lbase = 0;
  }
  if (Rbase < 0) {
    Rbase = 0;
  }

}




void vumeter( int Lval, int Rval, int Lpeak, int Rpeak)
{
  int i;

  for (i = 0; i < line + line; i++) {
    strip.setPixelColor(i, c_empty);
  }

  //strip.setPixelColor(line - 1, c_basebottom);
  //strip.setPixelColor(line, c_basebottom);

  for (i = line - Lval; i < line; i++) {
    strip.setPixelColor(i, c_basebody);
//strip.setPixelColor(i, strip.Color( i, 0, 255 - i));
  }


  for (i = line; i < line + Rval; i++) {
    strip.setPixelColor(i, c_basebody);
  }


  if (Lval > 0) {
    strip.setPixelColor(line - Lval, c_basetop);
  }

  if (Rval > 0) {
    strip.setPixelColor(line + Rval - 1, c_basetop);
  }


  if (Lpeak > 0) {
    strip.setPixelColor(line - Lpeak - 1, c_peak);
  }

  if (Rpeak > 0) {
    strip.setPixelColor(line + Rpeak, c_peak);
  }

  strip.show();

}



