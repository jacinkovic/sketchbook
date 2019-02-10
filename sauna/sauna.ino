#include <Wire.h> //LCD
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int DS18S20_Pin = 2; //DS18S20 Signal pin on digital 2
OneWire ds(DS18S20_Pin); // on digital pin 2


const int nastTempMin = 400;
const int nastTempMax = 950;

int i;
long int skutTemp, nastTemp, casVyp;
long int casEnd;
int dvojbodkaStav = 0;


const long int getTempUpdateTimePeriod = 30 * 1000L;
long int getTempUpdateTime;
const long int releGoUpdateTimePeriod = 1000;
long int releGoUpdateTime;
const long int sendSerialUpdateTimePeriod = 1 * 1000L;
long int sendSerialUpdateTime;

const long AutoShutdownTime = 4.5 * 60 * 60L;
//const int InitNastTemp = 65;

int pinKeyGo = A0;
int pinKeyHore = A1;
int pinKeyDole = A2;
int pin380Rele = A3;

const int PinReleSpirala1 = 5;
const int PinReleSpirala2 = 6;
const int PinReleSpirala3 = 7;
const int PinReleA = 8;
const int PinReleB = 9;


int ohrevSpirala[5] = {
  0, 0, 0, 0
};
int ohrevSpiralaSum = 0;
int ohrevSpiralaPos = 1; //od prvej spiraly
int ohrevHstr = 0; //hysterezia v desatinach C
int ohrev3spiralyTemp = 5; //teplota pri ktorej zapina vsetky 3 spiraly
int ohrev2spiralyTemp = 0; //teplota pri ktorej zapina 2 spiraly
int ohrev1spiraluTemp = -5; //teplota pri ktorej zapina 2 spiraly

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
char bignumchars1[] = {
  4, 2, 4,  2, 4, 1,  4, 2, 4,  4, 2, 4,  4, 1, 4,  4, 2, 2,  4, 2, 4,  2, 2, 4,  4, 2, 4,  4, 2, 4
};
char bignumchars2[] = {
  4, 1, 4,  1, 4, 1,  3, 3, 4,  1, 3, 4,  4, 3, 4,  4, 3, 3,  4, 3, 3,  1, 1, 4,  4, 3, 4,  4, 3, 4
};
char bignumchars3[] = {
  4, 1, 4,  1, 4, 1,  4, 2, 2,  1, 2, 4,  2, 2, 4,  2, 2, 4,  4, 2, 4,  1, 1, 4,  4, 2, 4,  2, 2, 4
};
char bignumchars4[] = {
  4, 3, 4,  3, 4, 3,  4, 3, 3,  4, 3, 4,  1, 1, 4,  4, 3, 4,  4, 3, 4,  1, 1, 4,  4, 3, 4,  4, 3, 4
};




void setup()
{

  wdt_enable(WDTO_8S);

  lcd.begin(20, 4);
  lcd.noCursor();
  lcd.setCursor(5, 1);
  lcd.print(F("SAUNA 2015"));

  lcd.createChar(0, znak0);
  lcd.createChar(1, znak1);
  lcd.createChar(2, znak2);
  lcd.createChar(3, znak3);
  lcd.createChar(4, znak4);
  lcd.createChar(5, znak5);
  lcd.createChar(6, znak6);
  lcd.createChar(7, znak7);

  wdt_reset(); delay(1000);

  Serial.begin(115200);

  casEnd = 0;//hod v ms

  nastTemp = constrain(int(EEPROM.read(100)) * 10, nastTempMin, nastTempMax);

  for (i = 0; i < 10; i++) { //inicializacia ds18b20
    getTemp();
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

  sendSerialUpdateTime = millis() + sendSerialUpdateTimePeriod;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Najprv skontroluj:"));
  lcd.setCursor(0, 1);
  lcd.print(F("- je pec cista?"));
  lcd.setCursor(0, 2);
  lcd.print(F("- je vetrak zavrety?"));
  lcd.setCursor(0, 3);
  lcd.print(F("- su dvere zavrete?"));
  wdt_reset(); delay(5000);
  wdt_reset();
  lcd.clear();

}

void loop()
{
  wdt_reset();

  if (millis() > getTempUpdateTime) {
    if (checkKey() == 0) {
      skutTemp = getTemp() * 10;
      if (skutTemp < 0) skutTemp = 0;  //orezanie od 0.0C do 99.9C
      if (skutTemp > 999) skutTemp = 999;
      vypisSkutTemp(skutTemp);
      getTempUpdateTime = millis() + getTempUpdateTimePeriod;
    }
    saveNastTemp();
  }


  doKey();
  vypisNastTemp(nastTemp);
  vypisStav();


  if (millis() > releGoUpdateTime) {
    releVypocet();
    releGo();
    vypisRele();
    releGoUpdateTime = millis() + releGoUpdateTimePeriod;
  }

  if (millis() > sendSerialUpdateTime) {
    sendSerial();
    sendSerialUpdateTime = millis() + sendSerialUpdateTimePeriod;
  }


}




void vypisNastTemp(int temp)
{
  lcd.setCursor(12, 0);
  lcd.write((byte)5);
  if (temp < 100) lcd.print(F(" "));
  lcd.print(temp / 10);
  lcd.write(B11011111); //stupen
};


void vypisCasVyp(int cas)
{
  cas = cas / 60; //sekundy nevypisujem
  int hodin = cas / 60;
  int minut = cas - hodin * 60;
  lcd.setCursor(12, 1);
  lcd.write((byte)7);
  lcd.print(hodin);
  lcd.print(F(":"));
  if (minut < 10) {
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
  vypisVelkeCislicu(c2, 0 + 4);
  lcd.setCursor(0 + 7, 3);
  lcd.write((byte)6); //bodka
  vypisVelkeCislicu(c3, 0 + 8);
  //lcd.setCursor(0 + 8, 3);
  //lcd.print(c3); //desatinne cislo
};

void vypisVelkeCislicu(int cislo, int x_pos)
{
  for (i = 0; i < 3; i++) {
    lcd.setCursor(x_pos + i, 0);
    lcd.write((byte)bignumchars1[3 * cislo + i]);
    lcd.setCursor(x_pos + i, 1);
    lcd.write((byte)bignumchars2[3 * cislo + i]);
    lcd.setCursor(x_pos + i, 2);
    lcd.write((byte)bignumchars3[3 * cislo + i]);
    lcd.setCursor(x_pos + i, 3);
    lcd.write((byte)bignumchars4[3 * cislo + i]);
  };
};



float getTemp() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    //Serial.println(F("CRC is not valid!"));
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    //Serial.print(F("Device is not recognized"));
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



int checkKey(void)
{

  if (analogRead(pinKeyGo) == 1023) {
    return (1);
  }
  if (analogRead(pinKeyHore) == 1023) {
    return (2);
  }
  if (analogRead(pinKeyDole) == 1023) {
    return (3);
  }

  return (0);
}

void doKey(void)
{

  if (analogRead(pinKeyHore) == 1023) {
    if (nastTemp < nastTempMax) {
      nastTemp = nastTemp + 10;
      vypisNastTemp(nastTemp);
      waitToReleaseKey();
      return;
    }
  }

  if (analogRead(pinKeyDole) == 1023) {
    if (nastTemp > nastTempMin) {
      nastTemp = nastTemp - 10;
      vypisNastTemp(nastTemp);
      waitToReleaseKey();
      return;
    }
  }

  if (analogRead(pinKeyGo) == 1023) { //vypnutie stlacenim

    if (casEnd > millis() / 1000) {
      casEnd = millis() / 1000;
    }
    else {
      casEnd = millis() / 1000 + AutoShutdownTime;
    }
    vypisStav();
    waitToReleaseKey();
    return;
  }

}

void waitToReleaseKey(void)
{
  const int delayKey = 500;

  int d = 0;
  do {
    delay(1);
    d++;
  }
  while (((analogRead(pinKeyGo) == 1023) ||
          (analogRead(pinKeyHore) == 1023) ||
          (analogRead(pinKeyDole) == 1023)) &&
         (d < delayKey)); //wait until release button

}




void ReleSpiralaZapni(void) {
  ohrevSpiralaPos++;
  if (ohrevSpiralaPos > 3) {
    ohrevSpiralaPos = 1;
  }
  ohrevSpirala[ohrevSpiralaPos] = 1;
  ohrevSpiralaSum++;
}

void ReleSpiralaVypni(void) {

  if (ohrevSpiralaSum == 1) { //jedna spirala
    ohrevSpirala[ohrevSpiralaPos] = 0;

    ohrevSpiralaPos++;
    if (ohrevSpiralaPos > 3) {
      ohrevSpiralaPos = 1;
    }
    ohrevSpiralaSum--;
  }
  else if (ohrevSpiralaSum == 2) { //dve
    if (ohrevSpiralaPos - 1 > 0) {
      ohrevSpirala[ohrevSpiralaPos - 1] = 0;
    }
    else {
      ohrevSpirala[3] = 0;
    }
    ohrevSpiralaSum--;
  }
  else if (ohrevSpiralaSum == 3) { //tri
    if (ohrevSpiralaPos == 1) {
      ohrevSpirala[2] = 0;
    }
    else if (ohrevSpiralaPos == 2) {
      ohrevSpirala[3] = 0;
    }
    else if (ohrevSpiralaPos == 3) {
      ohrevSpirala[1] = 0;
    }
    ohrevSpiralaSum--;
  }

}


void releVypocet(void)
{
  //rele
  if (casVyp > 0) {
    if ((nastTemp - skutTemp > ohrev3spiralyTemp + ohrevHstr) && (ohrevSpiralaSum < 3)) {
      ReleSpiralaZapni();
    }
    else if ((nastTemp - skutTemp > ohrev2spiralyTemp + ohrevHstr) && (ohrevSpiralaSum < 2)) {
      ReleSpiralaZapni();
    }
    else if ((nastTemp - skutTemp > ohrev1spiraluTemp + ohrevHstr) && (ohrevSpiralaSum < 1)) {
      ReleSpiralaZapni();
    }
    else if ((nastTemp - skutTemp < ohrev3spiralyTemp - ohrevHstr) && (ohrevSpiralaSum > 2)) {
      ReleSpiralaVypni();
    }
    else if ((nastTemp - skutTemp < ohrev2spiralyTemp - ohrevHstr) && (ohrevSpiralaSum > 1)) {
      ReleSpiralaVypni();
    }
    else if ((nastTemp - skutTemp < ohrev1spiraluTemp - ohrevHstr) && (ohrevSpiralaSum > 0)) {
      ReleSpiralaVypni();
    }
  }

  if (casVyp == 0) {
    if (ohrevSpiralaSum > 0) {
      ReleSpiralaVypni();
    }
  }


}



void releGo(void) {
  //rele
  if (ohrevSpirala[1] == 1) {
    analogWrite(PinReleSpirala1, 255);
  }
  else {
    analogWrite(PinReleSpirala1, 0);
  }
  if (ohrevSpirala[2] == 1) {
    analogWrite(PinReleSpirala2, 255);
  }
  else {
    analogWrite(PinReleSpirala2, 0);
  }
  if (ohrevSpirala[3] == 1) {
    analogWrite(PinReleSpirala3, 255);
  }
  else {
    analogWrite(PinReleSpirala3, 0);
  }

  //svetla
  if (casVyp > 0) {
    analogWrite(PinReleA, 255);
    analogWrite(PinReleB, 0);
  }
  if (casVyp == 0) {
    analogWrite(PinReleA, 0);
    analogWrite(PinReleB, 255);
  }

}


void vypisRele(void)
{
  lcd.setCursor(17, 0);
  if (ohrevSpirala[1] == 1) {
    lcd.write((byte)5);
  }
  else {
    lcd.print(F("_"));
  }
  if (ohrevSpirala[2] == 1) {
    lcd.write((byte)5);
  }
  else {
    lcd.print(F("_"));
  }
  if (ohrevSpirala[3] == 1) {
    lcd.write((byte)5);
  }
  else {
    lcd.print(F("_"));
  }

}


void getStav(void)
{

  //vypnute vypinacom
  if (analogRead(pin380Rele) == 0) {
    casEnd = millis() / 1000;
  }

  //vypnute casovacom
  casVyp = casEnd - millis() / 1000;
  if (casVyp < 0) {
    casVyp = 0;
  }

}



void vypisStav(void)
{
  getStav();

  lcd.setCursor(12, 1);
  if (casVyp == 0) {
    lcd.print(F("VYPNUTE"));
  }
  if (casVyp > 0) {
    vypisCasVyp(casVyp);
  }
}




void sendSerial(void)
{
  Serial.print ("S");
  Serial.print (" ");
  Serial.print (skutTemp);
  Serial.print (" ");
  Serial.print (nastTemp);
  Serial.print (" ");
  Serial.print (casVyp / 60);
  Serial.print (" ");
  Serial.print (ohrevSpirala[1]+ohrevSpirala[2]+ohrevSpirala[3]);
  Serial.print (" ");
  Serial.print (millis() / 1000);
  Serial.println ();
}





void saveNastTemp(void) {
  int address = 110;
  int value = nastTemp / 10;

  byte old = EEPROM.read(address);

  if (old != value) {
    EEPROM.write(address, value);
    //Serial.print(F("NastTemp saved to EEPROM saved at: "));
    //Serial.print(address);
    //Serial.print(F(" value: "));
    //Serial.println(value);

  }
}













































































