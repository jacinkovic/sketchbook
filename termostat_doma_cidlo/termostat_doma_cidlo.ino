#include <VirtualWire.h>
#include <avr/wdt.h>
#include "DHT.h"
#include <OneWire.h>

const byte sensor_cislo = '1';

unsigned int time;

int DS18S20_Pin = 2;
OneWire ds_Temp(DS18S20_Pin);


const int tx433MHz_pin = 12;
byte count433MHz = 1;

const unsigned int tx433MHz_UpdateTimePeriod = 60;  //in sec
unsigned int tx433MHzUpdateTime;

int errFlag;

int RoomTemp, RoomHumidity, RoomTempDHT, RoomDHTTempIndex;

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(115200);
  Serial.println(F("setup();"));

  init433MHz();

  dht.begin();

  randomSeed(analogRead(0));

  for(int i=0;i<10; i++){ //initialise ds18b20
    wdt_reset();  
    get_ds_Temp();
  }

  Serial.print(F("sensor_cislo = "));
  Serial.println(sensor_cislo-48);

  Serial.println(F("loop();"));

}

byte count = 1;

void loop()
{

  wdt_reset();  

  time = (unsigned long)(millis() / 1000);

  if(time > tx433MHzUpdateTime){
    Serial.println(F("Reading:"));
    LED(1);
    errFlag = 0;

    Serial.print(F("DHT:"));
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(t) || isnan(h)) {
      Serial.println(F("Failed to read from DHT"));
      errFlag = 1;
    } 
    else {
      RoomTempDHT = 10* t;
      RoomHumidity = 10* h;
      float hic = dht.computeHeatIndex(t, h, false);
      RoomDHTTempIndex = 10* hic;

      Serial.print(t);
      Serial.print(F("C "));
      Serial.print(h);
      Serial.println(F("%\t"));
      Serial.print(hic);
      Serial.println(F("C heatindex\t"));
    }

    Serial.print(F("DS18B20:"));
    float dsTemp = get_ds_Temp();
    if(dsTemp == -1000){
      Serial.println(F("Failed to read from DS18B20"));  
      errFlag = 1;
    }
    else{
      RoomTemp = 10 * dsTemp;
      Serial.print(dsTemp);
      Serial.println(F("C "));
    }

    Serial.println(F("Sending over 433MHz"));
    for(int i=0; i<1; i++){
      tx433MHz();
      delay(300);
    }
    tx433MHzUpdateTime = time + tx433MHz_UpdateTimePeriod - random(10);
    LED(0);
    Serial.println(F("Sent"));
  }

  delay(1000);

}




void init433MHz(void)
{
  // Initialise the IO and ISR
  vw_set_tx_pin(tx433MHz_pin);
  //vw_set_rx_pin(receive_pin);
  //vw_set_ptt_pin(transmit_en_pin);
  //vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);       // Bits per sec  
}



void tx433MHz(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '       };

  msg[0]='T';
  msg[1]='H';
  msg[2]='S';
  msg[3]= sensor_cislo;

  msg[4]= RoomTemp / 10;
  msg[5]= RoomTemp % 10;
  msg[6]= RoomHumidity / 10;
  msg[7]= RoomHumidity % 10;
  msg[8]= RoomTempDHT / 10;
  msg[9]= RoomTempDHT % 10;

  msg[10]= errFlag; 

  // replace chr 11 with count (#)
  msg[11] = count433MHz;
  //Serial.println(msg,DEC);
  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  count433MHz++;
}


void LED(int value){
  const byte led_pin=13;

  if(value>0){
    value=255;
  }
  pinMode(led_pin, OUTPUT);
  analogWrite(led_pin, value);
}



float get_ds_Temp(){
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

  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print(F("ERROR: CRC didn't match\n"));
    return -1000;
  }

  return TemperatureSum;

}





















