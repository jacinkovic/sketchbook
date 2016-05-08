#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>
#include <OneWire.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


int DS18S20_Pin = 2;
OneWire ds_Temp(DS18S20_Pin);

void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(9600);

  lcd.begin(16, 2);              // initialize the lcd
  lcd.clear();
  lcd.noCursor();
  lcd.backlight();

  lcd.home ();                   // go home
  lcd.print("Temperature");
  //lcd.setCursor ( 0, 1 );        // go to the next line

  for (int i = 0; i < 10; i++) { //initialise ds18b20
    wdt_reset();
    get_ds_Temp();
  }

}

void loop()
{
  wdt_reset();

  lcd.setCursor (0, 1);        // go to the next line

  float dsTemp = get_ds_Temp();
  if (dsTemp == -1000) {
    //oops!
    lcd.print("Error reading!");
  }
  else {
    //RoomTemp = 10 * dsTemp;
    Serial.println(dsTemp);
    lcd.print(dsTemp);
    lcd.print(" C            ");
  }


  delay(1000);

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


