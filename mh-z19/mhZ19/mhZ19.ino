#include <Wire.h> 
#include <avr/wdt.h>
#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

#define PIN_RESET 7  // LCD RST .... Pin 1
#define PIN_SCE   6  // LCD CS  .... Pin 3
#define PIN_DC    5  // LCD Dat/Com. Pin 5
#define PIN_SDIN  4  // LCD SPIDat . Pin 6
#define PIN_SCLK  3  // LCD SPIClk . Pin 4
// LCD Gnd .... Pin 2
// LCD Vcc .... Pin 8
// LCD Vlcd ... Pin 7

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_X     84
#define LCD_Y     48
#define LCD_CMD   0


static const byte ASCII[][5] =
{
  {
    0x00, 0x00, 0x00, 0x00, 0x00                              } // 20  
  ,{
    0x00, 0x00, 0x5f, 0x00, 0x00                              } // 21 !
  ,{
    0x00, 0x07, 0x00, 0x07, 0x00                              } // 22 "
  ,{
    0x14, 0x7f, 0x14, 0x7f, 0x14                              } // 23 #
  ,{
    0x24, 0x2a, 0x7f, 0x2a, 0x12                              } // 24 $
  ,{
    0x23, 0x13, 0x08, 0x64, 0x62                              } // 25 %
  ,{
    0x36, 0x49, 0x55, 0x22, 0x50                              } // 26 &
  ,{
    0x00, 0x05, 0x03, 0x00, 0x00                              } // 27 '
  ,{
    0x00, 0x1c, 0x22, 0x41, 0x00                              } // 28 (
  ,{
    0x00, 0x41, 0x22, 0x1c, 0x00                              } // 29 )
  ,{
    0x14, 0x08, 0x3e, 0x08, 0x14                              } // 2a *
  ,{
    0x08, 0x08, 0x3e, 0x08, 0x08                              } // 2b +
  ,{
    0x00, 0x50, 0x30, 0x00, 0x00                              } // 2c ,
  ,{
    0x08, 0x08, 0x08, 0x08, 0x08                              } // 2d -
  ,{
    0x00, 0x60, 0x60, 0x00, 0x00                              } // 2e .
  ,{
    0x20, 0x10, 0x08, 0x04, 0x02                              } // 2f /
  ,{
    0x3e, 0x51, 0x49, 0x45, 0x3e                              } // 30 0
  ,{
    0x00, 0x42, 0x7f, 0x40, 0x00                              } // 31 1
  ,{
    0x42, 0x61, 0x51, 0x49, 0x46                              } // 32 2
  ,{
    0x21, 0x41, 0x45, 0x4b, 0x31                              } // 33 3
  ,{
    0x18, 0x14, 0x12, 0x7f, 0x10                              } // 34 4
  ,{
    0x27, 0x45, 0x45, 0x45, 0x39                              } // 35 5
  ,{
    0x3c, 0x4a, 0x49, 0x49, 0x30                              } // 36 6
  ,{
    0x01, 0x71, 0x09, 0x05, 0x03                              } // 37 7
  ,{
    0x36, 0x49, 0x49, 0x49, 0x36                              } // 38 8
  ,{
    0x06, 0x49, 0x49, 0x29, 0x1e                              } // 39 9
  ,{
    0x00, 0x36, 0x36, 0x00, 0x00                              } // 3a :
  ,{
    0x00, 0x56, 0x36, 0x00, 0x00                              } // 3b ;
  ,{
    0x08, 0x14, 0x22, 0x41, 0x00                              } // 3c <
  ,{
    0x14, 0x14, 0x14, 0x14, 0x14                              } // 3d =
  ,{
    0x00, 0x41, 0x22, 0x14, 0x08                              } // 3e >
  ,{
    0x02, 0x01, 0x51, 0x09, 0x06                              } // 3f ?
  ,{
    0x32, 0x49, 0x79, 0x41, 0x3e                              } // 40 @
  ,{
    0x7e, 0x11, 0x11, 0x11, 0x7e                              } // 41 A
  ,{
    0x7f, 0x49, 0x49, 0x49, 0x36                              } // 42 B
  ,{
    0x3e, 0x41, 0x41, 0x41, 0x22                              } // 43 C
  ,{
    0x7f, 0x41, 0x41, 0x22, 0x1c                              } // 44 D
  ,{
    0x7f, 0x49, 0x49, 0x49, 0x41                              } // 45 E
  ,{
    0x7f, 0x09, 0x09, 0x09, 0x01                              } // 46 F
  ,{
    0x3e, 0x41, 0x49, 0x49, 0x7a                              } // 47 G
  ,{
    0x7f, 0x08, 0x08, 0x08, 0x7f                              } // 48 H
  ,{
    0x00, 0x41, 0x7f, 0x41, 0x00                              } // 49 I
  ,{
    0x20, 0x40, 0x41, 0x3f, 0x01                              } // 4a J
  ,{
    0x7f, 0x08, 0x14, 0x22, 0x41                              } // 4b K
  ,{
    0x7f, 0x40, 0x40, 0x40, 0x40                              } // 4c L
  ,{
    0x7f, 0x02, 0x0c, 0x02, 0x7f                              } // 4d M
  ,{
    0x7f, 0x04, 0x08, 0x10, 0x7f                              } // 4e N
  ,{
    0x3e, 0x41, 0x41, 0x41, 0x3e                              } // 4f O
  ,{
    0x7f, 0x09, 0x09, 0x09, 0x06                              } // 50 P
  ,{
    0x3e, 0x41, 0x51, 0x21, 0x5e                              } // 51 Q
  ,{
    0x7f, 0x09, 0x19, 0x29, 0x46                              } // 52 R
  ,{
    0x46, 0x49, 0x49, 0x49, 0x31                              } // 53 S
  ,{
    0x01, 0x01, 0x7f, 0x01, 0x01                              } // 54 T
  ,{
    0x3f, 0x40, 0x40, 0x40, 0x3f                              } // 55 U
  ,{
    0x1f, 0x20, 0x40, 0x20, 0x1f                              } // 56 V
  ,{
    0x3f, 0x40, 0x38, 0x40, 0x3f                              } // 57 W
  ,{
    0x63, 0x14, 0x08, 0x14, 0x63                              } // 58 X
  ,{
    0x07, 0x08, 0x70, 0x08, 0x07                              } // 59 Y
  ,{
    0x61, 0x51, 0x49, 0x45, 0x43                              } // 5a Z
  ,{
    0x00, 0x7f, 0x41, 0x41, 0x00                              } // 5b [
  ,{
    0x02, 0x04, 0x08, 0x10, 0x20                              } // 5c ¥
  ,{
    0x00, 0x41, 0x41, 0x7f, 0x00                              } // 5d ]
  ,{
    0x04, 0x02, 0x01, 0x02, 0x04                              } // 5e ^
  ,{
    0x40, 0x40, 0x40, 0x40, 0x40                              } // 5f _
  ,{
    0x00, 0x01, 0x02, 0x04, 0x00                              } // 60 `
  ,{
    0x20, 0x54, 0x54, 0x54, 0x78                              } // 61 a
  ,{
    0x7f, 0x48, 0x44, 0x44, 0x38                              } // 62 b
  ,{
    0x38, 0x44, 0x44, 0x44, 0x20                              } // 63 c
  ,{
    0x38, 0x44, 0x44, 0x48, 0x7f                              } // 64 d
  ,{
    0x38, 0x54, 0x54, 0x54, 0x18                              } // 65 e
  ,{
    0x08, 0x7e, 0x09, 0x01, 0x02                              } // 66 f
  ,{
    0x0c, 0x52, 0x52, 0x52, 0x3e                              } // 67 g
  ,{
    0x7f, 0x08, 0x04, 0x04, 0x78                              } // 68 h
  ,{
    0x00, 0x44, 0x7d, 0x40, 0x00                              } // 69 i
  ,{
    0x20, 0x40, 0x44, 0x3d, 0x00                              } // 6a j 
  ,{
    0x7f, 0x10, 0x28, 0x44, 0x00                              } // 6b k
  ,{
    0x00, 0x41, 0x7f, 0x40, 0x00                              } // 6c l
  ,{
    0x7c, 0x04, 0x18, 0x04, 0x78                              } // 6d m
  ,{
    0x7c, 0x08, 0x04, 0x04, 0x78                              } // 6e n
  ,{
    0x38, 0x44, 0x44, 0x44, 0x38                              } // 6f o
  ,{
    0x7c, 0x14, 0x14, 0x14, 0x08                              } // 70 p
  ,{
    0x08, 0x14, 0x14, 0x18, 0x7c                              } // 71 q
  ,{
    0x7c, 0x08, 0x04, 0x04, 0x08                              } // 72 r
  ,{
    0x48, 0x54, 0x54, 0x54, 0x20                              } // 73 s
  ,{
    0x04, 0x3f, 0x44, 0x40, 0x20                              } // 74 t
  ,{
    0x3c, 0x40, 0x40, 0x20, 0x7c                              } // 75 u
  ,{
    0x1c, 0x20, 0x40, 0x20, 0x1c                              } // 76 v
  ,{
    0x3c, 0x40, 0x30, 0x40, 0x3c                              } // 77 w
  ,{
    0x44, 0x28, 0x10, 0x28, 0x44                              } // 78 x
  ,{
    0x0c, 0x50, 0x50, 0x50, 0x3c                              } // 79 y
  ,{
    0x44, 0x64, 0x54, 0x4c, 0x44                              } // 7a z
  ,{
    0x00, 0x08, 0x36, 0x41, 0x00                              } // 7b {
  ,{
    0x00, 0x00, 0x7f, 0x00, 0x00                              } // 7c |
  ,{
    0x00, 0x41, 0x36, 0x08, 0x00                              } // 7d }
  ,{
    0x10, 0x08, 0x08, 0x10, 0x08                              } // 7e ←
  ,{
    0x00, 0x06, 0x09, 0x09, 0x06                              } // 7f →
};




void LcdCharacter(char character)
{
  LcdWrite(LCD_D, 0x00);
  for (int index = 0; index < 5; index++)
  {
    LcdWrite(LCD_D, ASCII[character - 0x20][index]);
  }
  LcdWrite(LCD_D, 0x00);
}

void LcdClear(void)
{
  for (int index = 0; index < LCD_X * LCD_Y / 8; index++)
  {
    LcdWrite(LCD_D, 0x00);
  }
}

void LcdInitialise(void)
{
  pinMode(PIN_SCE,   OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC,    OUTPUT);
  pinMode(PIN_SDIN,  OUTPUT);
  pinMode(PIN_SCLK,  OUTPUT);

  digitalWrite(PIN_RESET, LOW);
  // delay(1);
  digitalWrite(PIN_RESET, HIGH);

  LcdWrite( LCD_CMD, 0x21 );  // LCD Extended Commands.
  LcdWrite( LCD_CMD, 0xA2 );  // Set LCD Vop (Contrast). //B1 A2
  LcdWrite( LCD_CMD, 0x04 );  // Set Temp coefficent. //0x04
  LcdWrite( LCD_CMD, 0x14 );  // LCD bias mode 1:48. //0x13
  LcdWrite( LCD_CMD, 0x0C );  // LCD in normal mode. 0x0d for inverse
  LcdWrite(LCD_C, 0x20);
  LcdWrite(LCD_C, 0x0C);
}

#define NUM_CHARS 12

void LcdString(char *characters)
{
  int n = NUM_CHARS;

  while (*characters)
  {
    LcdCharacter(*characters++);
    n--;

    if(n==0){
      n = NUM_CHARS;
    }  
  }

  for(; n>0; n--)
  {
    LcdCharacter(0x20);
  }

}


void LcdWrite(byte dc, byte data)
{
  digitalWrite(PIN_DC, dc);
  digitalWrite(PIN_SCE, LOW);
  shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
  digitalWrite(PIN_SCE, HIGH);
}

// gotoXY routine to position cursor 
// x - range: 0 to 84
// y - range: 0 to 5

void gotoXY(int x, int y)
{
  LcdWrite( 0, 0x80 | x);  // Column.
  LcdWrite( 0, 0x40 | y);  // Row.  

}

#define MHZ19_INPUT_PIN 8

const unsigned long EtherQuery_Timeout = 15 * 60000L;
unsigned long EtherQueryLast;

const unsigned long LCD_Refresh = 1 * 1000L;
unsigned long LCDRefreshLast;

const unsigned long MeasureTime_Period = 29 * 1000L;
unsigned long MeasureTimeLast;

int co2;

float MHZ19_readPpm() {
  unsigned long lowCounter = 0;
  unsigned long highCounter = 0;

  // wait for low
  while (1 == digitalRead(MHZ19_INPUT_PIN)) {
  }

  // wait for high
  while (0 == digitalRead(MHZ19_INPUT_PIN)) {
  }

  // count high iterations
  while (1 == digitalRead(MHZ19_INPUT_PIN)) {
    highCounter++;
  }

  // count low iterations
  while (0 == digitalRead(MHZ19_INPUT_PIN)) {
    lowCounter++;
  }

  float ppm = 5000.0 * (1002.0 * highCounter - 2.0 * lowCounter) / 1000.0
    / (highCounter + lowCounter);

  //  Serial.print("high = ");
  //  Serial.print(highCounter);
  //  Serial.print(" ; low = ");
  //  Serial.print(lowCounter);
  //  Serial.print(" ; high+low = ");
  //  Serial.print(highCounter + lowCounter);
  //  Serial.print(" ; ppm = ");
  //  Serial.print(ppm);
  //  Serial.println();

  return ppm;
}



void setup()
{  
  char str [24];
  //
  //  Serial.begin(9600);
  //  Serial.println("setup();");

  wdt_enable(WDTO_8S);

  pinMode ( MHZ19_INPUT_PIN, INPUT );

  //Serial.println("eth");

  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x36                                  };
  IPAddress myIP(192, 168, 1, 206);
  Ethernet.begin(mac, myIP);
  server.begin();

  //Serial.println("lcd");

  LcdInitialise();
  LcdClear();
  gotoXY(0,0);
  sprintf(str, "CO2");   
  LcdString (str);

  //Serial.println("co2");

  //  wdt_reset();
  co2 = MHZ19_readPpm();
  //  wdt_reset();

  //Serial.println("loop();");


#if 0
  while (1) {
    int i = digitalRead(MHZ19_INPUT_PIN);
    //Serial.print(i);
  }
#endif

}

void loop(void)
{
  static int i;
  char str [24];

  //ETHERNET PART
  if (EthernetClient client = server.available())
  {
    //Serial.print(F("eth. "));
    client.print(co2);
    client.stop();
    //Serial.println(co2);
    EtherQueryLast = millis();
  }

  if ((long)(millis() - MeasureTimeLast > MeasureTime_Period))
  {
    //Serial.print(F("measure. "));
    co2 = MHZ19_readPpm();
    //Serial.println(co2);
    //Serial.println(F(" ppm"));
    MeasureTimeLast = millis();
  }

   //reset ak sa ethernet neozyva
  if ((long)(millis() - EtherQueryLast > EtherQuery_Timeout))
  {
    //Serial.println(F("EtherQueryLastTimeout"));
    while (1);
  }

  //refresh lcd
  if ((long)(millis() - LCDRefreshLast > LCD_Refresh))
  {

    gotoXY(0,2);
    sprintf(str, " %d ppm    ", co2);   
    LcdString (str);

 
    gotoXY(0,5);
    int nb = (millis() / 1000) % 12;
    sprintf(str, " %d  ", nb);   
    //LcdString (str);

    for(i=0; i<12; i++){
      if(i<=nb){
        LcdCharacter(0x2e);
      } 
      else {
        LcdCharacter(0x20);
      }

    }

    LCDRefreshLast = millis();
  }


  wdt_reset();

}


















