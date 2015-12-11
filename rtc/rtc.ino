// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

RTC_DS1307 RTC;
void setup () {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
}
void loop () {
  DateTime now = RTC.now();
  
  lcd.setCursor(0,1);
  if(now.hour()<10){ lcd.print("0");} lcd.print(now.hour());
  lcd.print(":");
  if(now.minute()<10){ lcd.print("0");} lcd.print(now.minute());
  lcd.print(":");
  if(now.second()<10){ lcd.print("0");} lcd.print(now.second());
  lcd.setCursor(0,0);
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year()); 
   
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print(" unixtime = ");
  Serial.print(now.unixtime());
  Serial.println();
  delay(1000);
}
