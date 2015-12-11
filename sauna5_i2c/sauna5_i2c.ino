#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include "DHT.h"
#include <VirtualWire.h>  //433
#include <EEPROM.h>

const int pin433MHz = 12;
byte count433MHz = 1;

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x20 for a 16 chars and 2 line display

int DS18S20_Pin = 2; //DS18S20 Signal pin on digital 2
OneWire ds(DS18S20_Pin); // on digital pin 2
int DS18S20Room_Pin = 10; //DS18S20 Signal pin on digital 2
OneWire dsRoom(DS18S20Room_Pin); // on digital pin 2


#define DHTPIN 11 // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

const int nastTempMin = 400;
const int nastTempMax = 950;

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
//const int InitNastTemp = 65;

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

  lcd.init();
  lcd.noCursor();
  lcd.createChar(0, znak0);
  lcd.createChar(1, znak1);
  lcd.createChar(2, znak2);
  lcd.createChar(3, znak3);
  lcd.createChar(4, znak4);
  lcd.createChar(5, znak5);
  lcd.createChar(6, znak6);
  lcd.createChar(7, znak7);
  lcd.backlight();
  //lcd.noBacklight();

  lcd.setCursor(0,0);
  lcd.print(F("Sauna 2015"));

  Serial.begin(115200);

  casEnd = 0;//hod v ms

  nastTemp = constrain(int(EEPROM.read(100)) * 10, nastTempMin, nastTempMax); 

  for (i = 0; i<10; i++){ //inicializacia ds18b20
    getTemp();
    getTempRoom();
  }
  getTempUpdateTime = 0;

  pinMode(pinKeyGo, INPUT);  
  pinMode(pinKeyHore, INPUT);  
  pinMode(pinKeyDole, INPUT);  

  pinMode(PinReleSpirala1, OUTPUT);
  pinMode(PinReleSpirala2, OUTPUT);
  pinMode(PinReleSpirala3, OUTPUT);
  pinMode(PinReleA, OUTPUT);
  pinMode(PinReleB, OUTPUT);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  init433MHz();
  send433MHzUpdateTime = millis() + send433MHzUpdateTimePeriod;

  buzzer(200);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Skontroluj !!"));
  lcd.setCursor(0,1);
  lcd.print(F("1. pec cista"));
  lcd.setCursor(0,2);
  lcd.print(F("2. vetracka zavreta"));
  lcd.setCursor(0,3);
  lcd.print(F("3. dvere zavrete"));
  delay(10000);
  lcd.clear();

}

void loop() 
{ 
  wdt_reset();

  if(millis() > getTempUpdateTime){
    if(checkKey()==0){
      skutTemp = getTemp() * 10;
      if (skutTemp < 0) skutTemp = 0;  //orezanie od 0.0C do 99.9C
      if (skutTemp > 999) skutTemp = 999;
      vypisSkutTemp(skutTemp);
      getTempUpdateTime = millis() + getTempUpdateTimePeriod;

      float h = dht.readHumidity();
      float t = dht.readTemperature();

      if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT");
      } 
      else {
        RoomTemp=getTempRoom();
        RoomHumidity=h;
        vypisRoomTemp(RoomTemp);
        vypisRoomHumidity(RoomHumidity);
      }
    }
    saveNastTemp();
  }


  doKey();
  vypisNastTemp(nastTemp); 
  vypisStav();


  if(millis() > releGoUpdateTime){
    releVypocet();
    releGo();
    vypisRele();
    releGoUpdateTime = millis() + releGoUpdateTimePeriod;
  }

  if(millis() > send433MHzUpdateTime){
    send433MHz();
    send433MHzUpdateTime = millis() + send433MHzUpdateTimePeriod;
  }


}




void vypisNastTemp(int temp)
{
  lcd.setCursor(12, 0);
  lcd.write((byte)5); 
  if (temp<100) lcd.print(F(" "));
  lcd.print(temp/10);
  lcd.write(B11011111); //stupen
};

void vypisRoomTemp(int temp)
{
  lcd.setCursor(13, 3);
  if (temp<10) lcd.print(F("0"));
  lcd.print(temp);
  lcd.write(B11011111); //stupen
};

void vypisRoomHumidity(int hum)
{
  lcd.setCursor(17, 3);
  if (hum<10) lcd.print(F("0"));
  lcd.print(hum);
  lcd.print(F("%")); 
};

void vypisCasVyp(int cas)
{
  cas = cas/60;  //sekundy nevypisujem
  int hodin = cas/60;
  int minut = cas - hodin*60; 
  lcd.setCursor(12, 1);
  lcd.write((byte)7);
  lcd.print(hodin);
  lcd.print(F(":"));  
  if(minut<10){ 
    lcd.print(F("0")); 
  };
  lcd.print(minut);
  lcd.print(F("  "));
};


void vypisSkutTemp(int cislo)
{
  int c1 = cislo / 100;
  int c2 = (cislo - 100 * c1) / 10;
  int c3 = cislo - (100 * c1 + 10 * c2);

  vypisVelkeCislicu(c1, 0);
  vypisVelkeCislicu(c2, 0+4);
  lcd.setCursor(0+7, 3);
  lcd.write((byte)6); //bodka 
  vypisVelkeCislicu(c3, 0+8);
};

void vypisVelkeCislicu(int cislo, int x_pos)
{
  for(i=0; i<3; i++){ 
    lcd.setCursor(x_pos+i, 0); 
    lcd.write((byte)bignumchars1[3*cislo+i]);
    lcd.setCursor(x_pos+i, 1);
    lcd.write((byte)bignumchars2[3*cislo+i]);    
    lcd.setCursor(x_pos+i, 2);
    lcd.write((byte)bignumchars3[3*cislo+i]);    
    lcd.setCursor(x_pos+i, 3);
    lcd.write((byte)bignumchars4[3*cislo+i]);    
  };
};



float getTemp(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println(F("CRC is not valid!"));
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print(F("Device is not recognized"));
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44); // start conversion

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}



float getTempRoom(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !dsRoom.search(addr)) {
    //no more sensors on chain, reset search
    dsRoom.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println(F("CRC is not valid!"));
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print(F("Device is not recognized"));
    return -1000;
  }

  dsRoom.reset();
  dsRoom.select(addr);
  dsRoom.write(0x44); // start conversion

  byte present = dsRoom.reset();
  dsRoom.select(addr);
  dsRoom.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = dsRoom.read();
  }

  dsRoom.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}


int checkKey(void)
{

  if(analogRead(pinKeyGo)==1023){ 
    return(1);
  }
  if(analogRead(pinKeyHore)==1023){ 
    return(2);
  }
  if(analogRead(pinKeyDole)==1023){ 
    return(3);
  }

  return(0);
}

void doKey(void)
{

  if(analogRead(pinKeyHore)==1023){
    if (nastTemp < nastTempMax){
      nastTemp = nastTemp+10;   
      vypisNastTemp(nastTemp); 
      waitToReleaseKey();
      return;
    }  
  }

  if(analogRead(pinKeyDole)==1023){
    if (nastTemp > nastTempMin){
      nastTemp = nastTemp-10;   
      vypisNastTemp(nastTemp); 
      waitToReleaseKey();
      return;
    }
  }  

  if(analogRead(pinKeyGo)==1023){  //vypnutie stlacenim

    if(casEnd > millis()/1000){
      casEnd = millis()/1000;
    }     
    else {
      casEnd = millis()/1000 + AutoShutdownTime; 
    }
    vypisStav();
    waitToReleaseKey();
    return;
  }  

}

void waitToReleaseKey(void)
{
  const int delayKey = 500;

  buzzer(100);

  int d=0;
  do {  
    delay(1);
    d++; 
  } 
  while(((analogRead(pinKeyGo)==1023) || 
    (analogRead(pinKeyHore)==1023) ||
    (analogRead(pinKeyDole)==1023)) &&
    (d<delayKey)); //wait until release button

}




void ReleSpiralaZapni(void){
  ohrevSpiralaPos++;
  if(ohrevSpiralaPos>3){
    ohrevSpiralaPos=1;
  }
  ohrevSpirala[ohrevSpiralaPos]=1;
  ohrevSpiralaSum++;
}

void ReleSpiralaVypni(void){

  if(ohrevSpiralaSum==1){    //jedna spirala
    ohrevSpirala[ohrevSpiralaPos]=0;

    ohrevSpiralaPos++;
    if(ohrevSpiralaPos>3){ 
      ohrevSpiralaPos=1;
    }  
    ohrevSpiralaSum--;
  }
  else if(ohrevSpiralaSum==2){  //dve
    if(ohrevSpiralaPos-1>0){
      ohrevSpirala[ohrevSpiralaPos-1]=0;
    }
    else{
      ohrevSpirala[3]=0;
    }
    ohrevSpiralaSum--;
  }
  else if(ohrevSpiralaSum==3){  //tri
    if(ohrevSpiralaPos==1){
      ohrevSpirala[2]=0;
    }    
    else if(ohrevSpiralaPos==2){
      ohrevSpirala[3]=0;
    }    
    else if(ohrevSpiralaPos==3){
      ohrevSpirala[1]=0;
    }
    ohrevSpiralaSum--;
  }

}


void releVypocet(void)
{
  //rele
  if(casVyp>0){
    if((nastTemp-skutTemp>ohrev3spiralyTemp+ohrevHstr) && (ohrevSpiralaSum<3)){ 
      ReleSpiralaZapni();
    } 
    else if((nastTemp-skutTemp>ohrev2spiralyTemp+ohrevHstr) && (ohrevSpiralaSum<2)){ 
      ReleSpiralaZapni();
    }
    else if((nastTemp-skutTemp>ohrev1spiraluTemp+ohrevHstr) && (ohrevSpiralaSum<1)){ 
      ReleSpiralaZapni();
    }
    else if((nastTemp-skutTemp<ohrev3spiralyTemp-ohrevHstr) && (ohrevSpiralaSum>2)){ 
      ReleSpiralaVypni();
    }
    else if((nastTemp-skutTemp<ohrev2spiralyTemp-ohrevHstr) && (ohrevSpiralaSum>1)){ 
      ReleSpiralaVypni();
    }
    else if((nastTemp-skutTemp<ohrev1spiraluTemp-ohrevHstr) && (ohrevSpiralaSum>0)){ 
      ReleSpiralaVypni();
    }
  } 

  if (casVyp==0){
    if (ohrevSpiralaSum>0){
      ReleSpiralaVypni();
    }
  }


}



void releGo(void){
  //rele
  if(ohrevSpirala[1]==1){
    analogWrite(PinReleSpirala1, 255);    
  } 
  else {
    analogWrite(PinReleSpirala1, 0);    
  }
  if(ohrevSpirala[2]==1){
    analogWrite(PinReleSpirala2, 255);    
  } 
  else {
    analogWrite(PinReleSpirala2, 0);    
  }
  if(ohrevSpirala[3]==1){
    analogWrite(PinReleSpirala3, 255);    
  } 
  else {
    analogWrite(PinReleSpirala3, 0);    
  }

  //svetla
  if (casVyp>0){
    analogWrite(PinReleA, 255);    
    analogWrite(PinReleB, 0);    
  } 
  if (casVyp==0){
    analogWrite(PinReleA, 0);    
    analogWrite(PinReleB, 255);    
  }

}


void vypisRele(void)
{
  lcd.setCursor(17, 0);
  if(ohrevSpirala[1]==1){     
    lcd.write((byte)5);  
  }
  else{ 
    lcd.print(F("_")); 
  } 
  if(ohrevSpirala[2]==1){     
    lcd.write((byte)5);  
  }
  else{ 
    lcd.print(F("_")); 
  } 
  if(ohrevSpirala[3]==1){     
    lcd.write((byte)5);  
  }
  else{ 
    lcd.print(F("_")); 
  }   

}


void getStav(void)
{

  //vypnute vypinacom
  if(analogRead(pin380Rele)==0){
    casEnd = millis()/1000;
  }

  //vypnute casovacom
  casVyp = casEnd - millis()/1000;
  if (casVyp<0){ 
    casVyp = 0;  
  }

}



void vypisStav(void)
{
  getStav();

  lcd.setCursor(12, 1);
  if (casVyp==0){ 
    lcd.print(F("VYPNUTE"));
  }
  if(casVyp>0){   
    vypisCasVyp(casVyp);
  }  
}



void buzzer(int duration)
{
  pinMode(PinBuzzer, OUTPUT);

  for(int i=0; i<duration; i++){
    analogWrite(PinBuzzer, 0);
    delay(1); 
    analogWrite(PinBuzzer, 255);
    delay(1);  
  }

  analogWrite(PinBuzzer, 255);

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


  Serial.println("Send433MHz");
  char msg[12] = {
    '5','6','7','.','d','k',' ','t','e','s','t','#'                                                        };

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
  //Serial.println(msg[1]);
  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  count433MHz++;
}





void saveNastTemp(void){
  int address = 100;
  int value = nastTemp / 10;
  
  byte old = EEPROM.read(address);

  if(old!=value){
    EEPROM.write(address, value);  
    Serial.print(F("NastTemp saved to EEPROM saved at: "));
    Serial.print(address);
    Serial.print(F(" value: "));
    Serial.println(value);

  }
}












































































