
#include <Adafruit_NeoPixel.h>

#define PIN 2   //D2
#define NUM_NEOPIXEL 120
int brightness = 100;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_NEOPIXEL, PIN, NEO_GRB + NEO_KHZ800);
// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

const int analogReadsNum = 10;
const int topSize = NUM_NEOPIXEL / 8;
const int mediumSize = NUM_NEOPIXEL / 2 - topSize;
const long inValDiv = 5; //5
const long peakSlow  = 1;  //10
const long baseSlow  = 10;  //1
const int analogInPinL = A7;
const int analogInPinR = A6;

long Lval, Rval;
long Lbase = 0, Rbase = 0;
long Lpeak = 0, Rpeak = 0;
uint32_t c_basetop, c_basebody, c_peak, c_empty;
int line;
int t;

uint32_t c_pix_def[NUM_NEOPIXEL];

class GetColor {
  public:
    uint8_t r;
    uint8_t g;
    uint8_t b;

    GetColor(uint32_t c) {
      r = (uint8_t)(c >> 16),
      g = (uint8_t)(c >>  8),
      b = (uint8_t)c;
    }
};

uint32_t GetRandomColor(void) {
  int r = random(255);
  int g = random(255 - r);
  int b = random(255 - r - g);
  return strip.Color(r, g, b);
}


void setup() {

  line = strip.numPixels() / 2;

//#define CYCLECOLOR

  c_basetop = strip.Color( 255, 0, 0);
  //c_basetop = strip.Color( 255, 255, 255);
  c_basebody = strip.Color( 255, 255, 255);
  c_basebody = strip.Color( 255, 255, 255);
  c_basebody = strip.Color( 0, 255, 0);
  //c_basebody = strip.Color( 255, 0, 0);
  //c_basebody = strip.Color( 0, 0, 255);
  //c_basetop = c_basebody;
  //c_basebody = strip.Color( 0, 255, 0);
  c_peak = strip.Color( 255, 255, 255);
  //c_peak = strip.Color( 0, 0, 255);
  //c_peak = strip.Color( 255, 0 , 0);
  //c_peak = strip.Color( 255, 255 , 0);
  //c_basetop = c_peak;
  c_empty = strip.Color( 0, 0, 0);
  //c_peak = c_basebody;

  strip.begin();
  strip.setBrightness(brightness);
  strip.show();

  init_c_pix_def();

  pinMode(analogInPinL, INPUT);
  pinMode(analogInPinR, INPUT);

  randomSeed(analogRead(8));
  //Serial.begin(115200);
}



void loop() {
#ifdef CYCLECOLOR
  t++;
  if (t > 100) {
    c_basebody = GetRandomColor();
    c_basetop = GetRandomColor();
    //c_peak = GetRandomColor();
    c_peak = c_basebody;
    init_c_pix_def();
    t = 0;
  }
#endif


  Lval = 0;
  Rval = 0;

  for (int i = 0; i < analogReadsNum; i++)
  {
    Lval = max(analogRead(analogInPinL), Lval);
    Rval = max(analogRead(analogInPinR), Rval);
  }

  //Lval = 255;
  //Rval = Lval;


  Lval = Lval * 10; //no need for floating point, do it by integer
  Rval = Rval * 10;

  Lval = Lval / inValDiv;
  Rval = Rval / inValDiv;

  if (Lval > line * 10) Lval = line * 10;
  if (Rval > line * 10) Rval = line * 10;

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

  vumeter3(Lbase, Rbase, Lpeak, Rpeak);


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



void vumeter3( int Lval, int Rval, int Lpeak, int Rpeak)
{
  int i;
  int tmppos;
  float in, di, d;

  Lval = Lval / 10; //back to real analog value
  Rval = Rval / 10; //back to real analog value
  Lpeak = Lpeak / 10; //back to real analog value
  Rpeak = Rpeak / 10; //back to real analog value

  for (i = 0; i < line - Lval; i++) {
    strip.setPixelColor(i, c_empty);
  }

  for (i = line + Rval; i < NUM_NEOPIXEL; i++) {
    strip.setPixelColor(i, c_empty);
  }


  in = Lval;
  d = line / in;
  di = 0;
  for (i = line - Lval; i < line; i++) {
    strip.setPixelColor(i, c_pix_def[(int)di]);
    di = di + d;
  }

  in = Rval;
  d = line / in;
  di = line;
  for (i = line; i < line + Rval; i++) {
    strip.setPixelColor(i, c_pix_def[(int)di]);
    di = di + d;
  }




  if (Lpeak > 0) {
    strip.setPixelColor(line - Lpeak - 1, c_peak);
  }

  if (Rpeak > 0) {
    strip.setPixelColor(line + Rpeak, c_peak);
  }

  strip.show();

}



void init_c_pix_def(void) {
  int i;
  int m, pos;
  float divfrom, divto;
  float rf, gf, bf;
  uint8_t r, g, b;

  for (i = 0; i < topSize; i++) {
    c_pix_def[i] = c_basetop;
  }

  GetColor colorFrom(c_basetop);
  GetColor colorTo(c_basebody);
  m = mediumSize;

  for (i = topSize; i < mediumSize + topSize; i++) {
    //c_pix_def[i] = c_basemedium;

    pos = i - topSize;
    if (pos == 0) {
      divfrom = 1;
      divto = 0;
    } else {
      divfrom = 1 - (float)pos / (float)m;
      divto = (float)pos / (float)m;
    }

    rf = (float)colorFrom.r * divfrom + (float)colorTo.r * divto;
    gf = (float)colorFrom.g * divfrom + (float)colorTo.g * divto;
    bf = (float)colorFrom.b * divfrom + (float)colorTo.b * divto;
    //rf=gf=bf=255;
    c_pix_def[i] = strip.Color((int)rf, (int)gf, (int)bf);
    c_pix_def[NUM_NEOPIXEL - i - 1] = strip.Color((int)rf, (int)gf, (int)bf);
  }

  for (i = mediumSize + topSize; i < NUM_NEOPIXEL - mediumSize - topSize; i++) {
    c_pix_def[i] = c_basebody;
  }

  for (i = NUM_NEOPIXEL - topSize; i < NUM_NEOPIXEL; i++) {
    c_pix_def[i] = c_basetop;
  }



}





