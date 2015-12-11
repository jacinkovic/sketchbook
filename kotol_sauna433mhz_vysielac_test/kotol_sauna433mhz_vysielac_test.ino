#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include "DHT.h"
#include <VirtualWire.h>  //433

const int pin433MHz = 2;
byte count433MHz = 1;

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x20 for a 16 chars and 2 line display

int DS18S20_Pin = 2; //DS18S20 Signal pin on digital 2
OneWire ds(DS18S20_Pin); // on digital pin 2
int DS18S20Room_Pin = 10; //DS18S20 Signal pin on digital 2
OneWire dsRoom(DS18S20Room_Pin); // on digital pin 2


#define DHTPIN 11 // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);



int i;
long int skutTemp, nastTemp, casVyp;
long int casEnd;
int dvojbodkaStav=0;
float RoomTemp, RoomHumidity;


const long int getTempUpdateTimePeriod = 30 *1000L;
long int getTempUpdateTime;
const long int releGoUpdateTimePeriod = 1000;
long int releGoUpdateTime;
const long int send433MHzUpdateTimePeriod = 30 *1000L;
long int send433MHzUpdateTime;

//const long AutoShutdownTime = 3 *60*60L;
const long AutoShutdownTime = 4* 60* 60L;
const int InitNastTemp = 70;

int pinKeyGo = A0;
int pinKeyHore = A1;
int pinKeyDole = A2;
int pin380Rele = A3;

const int PinBuzzer=4;

const int PinReleSpirala1 = 5;
const int PinReleSpirala2 = 6;
const int PinReleSpirala3 = 7;
const int PinReleA = 8;
const int PinReleB = 9;


int ohrevSpirala[5]={
  0,0,0,0};
int ohrevSpiralaSum=0;
int ohrevSpiralaPos=1; //od prvej spiraly
int ohrevHstr=0; //hysterezia v desatinach C
int ohrev3spiralyTemp=5; //teplota pri ktorej zapina vsetky 3 spiraly
int ohrev2spiralyTemp=0;  //teplota pri ktorej zapina 2 spiraly
int ohrev1spiraluTemp=-5;  //teplota pri ktorej zapina 2 spiraly

byte znak1[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
}; 



byte znak2[8] = {
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
}; 

byte znak3[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
}; 



byte znak4[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
}; 


byte znak5[8] = {
  B01001,
  B10010,
  B01001,
  B10010,  
  B00000,
  B11111,
  B11111,
  B00000,
}; 

byte znak0[8] = {
  B00111,
  B00101,
  B00111,
  B00000,  
  B00000,
  B00000,
  B00000,
  B00000,
}; 

byte znak6[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,  
  B00000,
  B01110,
  B01110,
}; 




byte znak7[8] = {
  B01110,
  B10101,
  B10101,
  B10111,  
  B10001,
  B01110,
  B00000,
  B00000,
}; 


//0       1       2       3       4       5       6       7       8       9
char bignumchars1[]={
  4,2,4,  2,4,1,  4,2,4,  4,2,4,  4,1,4,  4,2,2,  4,2,4,  2,2,4,  4,2,4,  4,2,4};
char bignumchars2[]={
  4,1,4,  1,4,1,  3,3,4,  1,3,4,  4,3,4,  4,3,3,  4,3,3,  1,1,4,  4,3,4,  4,3,4};
char bignumchars3[]={
  4,1,4,  1,4,1,  4,2,2,  1,2,4,  2,2,4,  2,2,4,  4,2,4,  1,1,4,  4,2,4,  2,2,4};
char bignumchars4[]={
  4,3,4,  3,4,3,  4,3,3,  4,3,4,  1,1,4,  4,3,4,  4,3,4,  1,1,4,  4,3,4,  4,3,4};



void setup() 
{

  wdt_enable(WDTO_8S);

  Serial.begin(115200);

  init433MHz();

  skutTemp=600;
  nastTemp=750;
  RoomTemp=20;
  RoomHumidity=40;
  casVyp=3600;
  ohrevSpirala[1]=random(1);

}

void loop() 
{ 
  wdt_reset();

  send433MHz();
  delay(3000);

}












void init433MHz(void)
{
  // Initialise the IO and ISR
  vw_set_tx_pin(pin433MHz);
  //vw_set_rx_pin(receive_pin);
  //vw_set_ptt_pin(transmit_en_pin);
  //vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);       // Bits per sec  
}


void send433MHz(void)
{
  //buzzer(1);

  skutTemp = skutTemp + random(-1,2);
  RoomTemp = RoomTemp + random(-1,+2);
  RoomHumidity = RoomHumidity + random(-1,+2);
  casVyp--;
  ohrevSpirala[1]=random(0,2);
  ohrevSpirala[2]=random(0,2);
  ohrevSpirala[3]=random(0,2);

  Serial.println("Send433MHz");
  char msg[12] = {
    '5','6','7','.','d','k',' ','t','e','s','t','#'                                                          };

  msg[0]='S';
  msg[1]=skutTemp / 10;
  msg[2]=skutTemp % 10;
  msg[3]=nastTemp / 10;
  msg[4]=RoomTemp;
  msg[5]=RoomHumidity;
  msg[6]=casVyp / 3600;
  msg[7]=(casVyp / 60) - (casVyp / 3600) *60;
  msg[8]=ohrevSpirala[1];
  msg[9]=ohrevSpirala[2];
  msg[10]=ohrevSpirala[3];

  // replace chr 11 with count (#)
  msg[11] = count433MHz;

  for(i=0;i<12; i++){
    Serial.print(msg[i], DEC);
    Serial.print(" ");
  }
  Serial.println();

  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  count433MHz++;
}



















































































