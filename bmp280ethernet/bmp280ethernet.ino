#include <Wire.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <UIPEthernet.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SENSNR 1

EthernetServer server = EthernetServer(80);

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

const unsigned long EtherQuery_Timeout = 15 * 60000L;
unsigned long EtherQueryLast;

const unsigned long MeasureTime_Period = 15 * 1000L;
unsigned long MeasureTimeLast;

//int DS18S20_Pin = 3;
//OneWire ds_Temp(DS18S20_Pin);

float temp_bme, humid_bme, pressure_bme, altitude_bme, temp_dew;


void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(115200);
  Serial.print(F("start.\n"));


#if SENSNR == 0
  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x47
  };
  IPAddress myIP(192, 168, 1, 207);
#elif SENSNR == 1
  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x48
  };
  IPAddress myIP(192, 168, 1, 208);
#elif SENSNR == 2
  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x4A
  };
  IPAddress myIP(192, 168, 1, 209);
#elif SENSNR == 3
  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x4b
  };
  IPAddress myIP(192, 168, 1, 210);
#elif SENSNR == 4
  uint8_t mac[6] = {
    0x74, 0x73, 0x71, 0x2D, 0x32, 0x4c
  };
  IPAddress myIP(192, 168, 1, 211);
#endif

  Ethernet.begin(mac, myIP);
  server.begin();

  bme.begin();
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF   );

  measure_bme();

  //  for (int i = 0; i < 10; i++) { //initialise ds18b20
  //    wdt_reset();
  //    //get_ds_Temp();
  //  }
  //


}

void loop()
{
  wdt_reset();

  //ETHERNET PART
  if (EthernetClient client = server.available())
  {
    Serial.print(F("eth.\n"));
    client.print(temp_bme);
    client.print(F(" "));
    client.print(humid_bme);
    client.print(F(" "));
    client.print(pressure_bme);
    client.print(F(" "));
    client.print(temp_dew);
    client.print(F(" "));
    client.print(altitude_bme);
    client.stop();
    EtherQueryLast = millis();
  }

  if ((long)(millis() - MeasureTimeLast > MeasureTime_Period))
  {

    measure_bme();

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

void measure_bme() {
  Serial.print(F("measuring:"));

  bme.takeForcedMeasurement(); // has no effect in normal mode
  temp_bme = bme.readTemperature(); //C
  humid_bme = bme.readHumidity(); //%
  pressure_bme = (bme.readPressure() / 100.0F); //hpa
  altitude_bme = bme.readAltitude(SEALEVELPRESSURE_HPA); //m
  temp_dew =  (temp_bme - (100 - humid_bme) / 5);   //  dewpoint calculation using Celsius value

  Serial.print(F(" temp_bme= "));
  Serial.print(temp_bme);
  Serial.print(F(" humid_bme= "));
  Serial.print(humid_bme);
  Serial.print(F(" pressure_bme= "));
  Serial.print(pressure_bme);
  Serial.print(F(" temp_dew= "));
  Serial.print(temp_dew);
  Serial.print(F(" altitude_bme= "));
  Serial.print(altitude_bme);
  Serial.print(F("\n"));

}
