#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

#include "DHT.h"

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const byte DHTPIN = 4; // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)

const unsigned long EtherQuery_Timeout = 15 * 60000L;
unsigned long EtherQueryLast;

const unsigned long MeasureTime_Period = 5 * 1000L;
unsigned long MeasureTimeLast;

int HumidSensorPin = A0; 

DHT dht(DHTPIN, DHTTYPE);

int DS18S20_Pin = 3;
OneWire ds_Temp(DS18S20_Pin);

int DHTTemp;
int DHTHumid;

int co2, humidout;

#define MHZ19_INPUT_PIN 88

#define pwmPin 2
int preheatSec = 120;
int prevVal = LOW;
long th, tl, h, l, ppm = 0;

void PWM_ISR() {
  long tt = millis();
  int val = digitalRead(pwmPin);
  
  if (val == HIGH) {    
    if (val != prevVal) {
      h = tt;
      tl = h - l;
      prevVal = val;
    }
  }  else {    
    if (val != prevVal) {
      l = tt;
      th = l - h;
      prevVal = val;
      ppm = 5000 * (th - 2) / (th + tl - 4);  
    }
  }
}


void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(9600);

  lcd.begin(16, 2);              // initialize the lcd
  lcd.clear();
  lcd.noCursor();
  lcd.backlight();
  lcd.home ();                   // go home
  lcd.print(F("Starting ..."));
  
  
  //pinMode ( MHZ19_INPUT_PIN, INPUT );
  pinMode(pwmPin, INPUT);
  attachInterrupt(0, PWM_ISR, CHANGE);  
 
  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x36                                  };
  IPAddress myIP(192, 168, 1, 206);
  Ethernet.begin(mac, myIP);
  server.begin();


  dht.begin();
  
  for (int i = 0; i < 10; i++) { //initialise ds18b20
    wdt_reset();
    get_ds_Temp();
  }
  
 
  
}

void loop()
{
  wdt_reset();

//ETHERNET PART
  if (EthernetClient client = server.available())
  {
    Serial.print(F("eth. "));
    client.print(co2);
    client.print(F(" "));
    client.print(humidout);
    client.stop();
    EtherQueryLast = millis();
  }

  if ((long)(millis() - MeasureTimeLast > MeasureTime_Period))
  {
    co2 = ppm;
    Serial.println(co2);
    Serial.println(F(" ppm"));

    float humid = analogRead(HumidSensorPin);
    humid = humid / 1023 * 5; //convert to voltage
    humid = humid - 0.958;  //remove zero offset
    humid = humid / 0.03068; //slope
    humidout = round(humid);
    Serial.println(humidout);
    
//  int dsTemp = round(get_ds_Temp());
//  if (dsTemp == -1000) {
//    lcd.print("Err");
//  }
//  else {
//    Serial.println(dsTemp);
//    lcd.print(dsTemp);
//    lcd.print("C____|");
//  }

  //getHumid();
//  Serial.println(DHTTemp);
//  Serial.println(DHTHumid);
//  lcd.setCursor (0, 1);        
//  lcd.print(DHTTemp);
//  lcd.print("C|");
//  lcd.print(DHTHumid);
//  lcd.print("%____|");

    lcd.setCursor (0, 0);        
    //lcd.print(F("CO2 "));
    lcd.print(co2);
    lcd.print(F("ppm    "));

    lcd.setCursor (10, 0);        
    //lcd.print(F("RH "));
    lcd.print(humidout);
    lcd.print(F("%  "));

    lcd.setCursor (0, 1);        
    lcd.print(F("192.168.1.206"));

    MeasureTimeLast = millis();
  }

  //reset ak sa ethernet neozyva
  if ((long)(millis() - EtherQueryLast > EtherQuery_Timeout))
  {
    Serial.println(F("EtherQueryLastTimeout"));
    while (1);
  }
  
}


float get_ds_Temp() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_Temp.search(addr)) {
    //no more sensors on chain, reset search
    ds_Temp.reset_search();
    Serial.println(F("no more sensors on chain, reset search!"));
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

  ds_Temp.reset();
  ds_Temp.select(addr);
  ds_Temp.write(0x44); // start conversion

  byte present = ds_Temp.reset();
  ds_Temp.select(addr);
  ds_Temp.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_Temp.read();
  }

  ds_Temp.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data, 8)) {
    Serial.print(F("ERROR: CRC didn't match\n"));
    return -1000;
  }

  return TemperatureSum;

}



void getHumid(void){
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(t) || isnan(h)) {
#ifdef DEBUG
    Serial.println(F("DHT: failed"));
#endif
  }
  else {
    DHTTemp = t;
    DHTHumid = h;
  }  

}



