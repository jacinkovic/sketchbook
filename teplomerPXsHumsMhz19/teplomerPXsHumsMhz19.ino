#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <UIPEthernet.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

EthernetServer server = EthernetServer(80);

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const unsigned long EtherQuery_Timeout = 15 * 60000L;
unsigned long EtherQueryLast;

const unsigned long MeasureTime_Period = 60 * 1000L;
unsigned long MeasureTimeLast;

int HumidSensorPin = A0;

//int DS18S20_Pin = 3;
//OneWire ds_Temp(DS18S20_Pin);

int co2, humidout;
int temp_bme, humid_bme, pressure_bme, altitude_bme;

#define MHZ19_INPUT_PIN 2
int preheatSec = 120;
int prevVal = LOW;
long th, tl, h, l, ppm = 0;

void PWM_ISR() {
  long tt = millis();
  int val = digitalRead(MHZ19_INPUT_PIN);

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


  pinMode(MHZ19_INPUT_PIN, INPUT);
  attachInterrupt(0, PWM_ISR, CHANGE);

  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x37
  };
  IPAddress myIP(192, 168, 1, 206);
  Ethernet.begin(mac, myIP);
  server.begin();

  bme.begin();
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

  for (int i = 0; i < 10; i++) { //initialise ds18b20
    wdt_reset();
    //get_ds_Temp();
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
    client.print(F(" "));
    client.print(temp_bme);
    client.print(F(" "));
    client.print(humid_bme);
    client.print(F(" "));
    client.print(pressure_bme);
    client.print(F(" "));
    client.print(altitude_bme);
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

    bme.takeForcedMeasurement(); // has no effect in normal mode

    temp_bme = bme.readTemperature(); //C
    humid_bme = bme.readHumidity(); //%
    pressure_bme = (bme.readPressure() / 100.0F); //hpa
    altitude_bme = bme.readAltitude(SEALEVELPRESSURE_HPA); //m

    /*
      int dsTemp = round(get_ds_Temp());
      if (dsTemp == -1000) {
       lcd.print("Err");
      }
      else {
       Serial.println(dsTemp);
       lcd.print(dsTemp);
       lcd.print("C____|");
      }
    */

    lcd.clear();

    lcd.setCursor (0, 0);
    //lcd.print(F("CO2 "));
    lcd.print(co2);
    lcd.print(F("ppm"));

    lcd.setCursor (8, 0);
    lcd.print(humidout);
    lcd.print(F("%"));

    lcd.setCursor (12, 0);
    lcd.print(humid_bme);
    lcd.print(F("%"));

    lcd.setCursor (0, 1);
    lcd.print(temp_bme);
    lcd.print(F("C"));


    lcd.setCursor (4, 1);
    lcd.print(pressure_bme);
    lcd.print(F("hPa"));

    lcd.setCursor (12, 1);
    lcd.print(altitude_bme);
    lcd.print(F("m"));

    //lcd.setCursor (0, 1);
    //lcd.print(F("192.168.1.206"));

    MeasureTimeLast = millis();
  }

  //reset ak sa ethernet neozyva
  if ((long)(millis() - EtherQueryLast > EtherQuery_Timeout))
  {
    Serial.println(F("EtherQueryLastTimeout"));
    while (1);
  }

}


/*
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
*/
