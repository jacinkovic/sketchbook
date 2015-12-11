#include <OneWire.h>
#include <avr/wdt.h>

#include <VirtualWire.h> //433MHz

#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

#include "DHT.h"

const byte DHTPIN = 2; // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

//433MhZ
const byte receive433MHz_pin = 9; //povodne 11
uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;

const byte DS18S20_Pin_Temp1 = 5; //DS18S20 Signal pin
const byte DS18S20_Pin_Temp2 = 6; //DS18S20 Signal pin
const byte DS18S20_Pin_Temp3 = 7; //DS18S20 Signal pin
const byte DS18S20_Pin_tempRoom = 4; //DS18S20 Signal pin

//Temperature chip i/o
OneWire ds_Temp1(DS18S20_Pin_Temp1);
OneWire ds_Temp2(DS18S20_Pin_Temp2);
OneWire ds_Temp3(DS18S20_Pin_Temp3);
OneWire ds_tempRoom(DS18S20_Pin_tempRoom);

//byte dsdata[12];
//byte dsaddr[8];
const int dsRetErrValue = -1000;
//float dsTemperatureSum;

int temp1;
int temp2;
int temp3;
int tempTC;
int tempRoom;
int humidRoom;
int gasZemPlyn;
int gasCO;


//TC
const byte analogTCPin = A4;
const float TC_m = 0.10371769557;
const float TC_b = 37.3700094445;
const byte TC_repeat = 100;
//float TC_val, TCValue = 0;

const byte analoggasZemPlynPin = A0;
const byte analoggasCOPin = A1;

//int gasZemPlyn_val, gasZemPlynValue = 0;
//int gasCO_val, gasCOValue = 0;

unsigned int time;
const unsigned int Measure_TimePeriod = 20; //repeat reading from sensors
unsigned int Measure_TimeAgain = 0;

const unsigned int EtherQueryTimeout = 15 * 60;
unsigned int EtherQueryLast;

const unsigned int ReceiveSaunaTimeout = 1 * 60;
unsigned int ReceiveSaunaValidTime;

//sauna
int saunaskutTemp, saunanastTemp;
int saunacasVyp;
byte saunaohrevSpirala[5] = {
  0, 0, 0, 0
};




void setup() {
  wdt_enable(WDTO_8S);

  Serial.begin(115200);

  Serial.println(F("setup();"));

  Serial.print(F("freeRam=")); Serial.println(freeRam());

  time = (unsigned long)(millis() / 1000);

  pinMode(analogTCPin, INPUT);

  dht.begin();

  vw_set_rx_pin(receive433MHz_pin);
  vw_setup(2000);	 // Bits per sec
  vw_rx_start();   // Start the receiver PLL running

  uint8_t mac[6] = {0x74, 0x69, 0x69, 0x2D, 0x32, 0x32};
  IPAddress myIP(192, 168, 1, 202);
  Ethernet.begin(mac, myIP);
  server.begin();

  Serial.print(F("freeRam=")); Serial.println(freeRam());

  EtherQueryLast = time + EtherQueryTimeout;

  wdt_reset(); //wait for ds18b20
  delay(2000);
  read_sensors();

  Serial.println(F("loop();"));
}

void loop()
{
  wdt_reset();

  time = (unsigned long)(millis() / 1000);

  checkReceiveSauna433MHz();

  if (time > Measure_TimeAgain)
  {
    Serial.print(F("freeRam=")); Serial.println(freeRam());

    read_sensors();
    //read_sensors();

    Serial.println(F("Measured."));
    Serial.print(temp1);
    Serial.print(F(" "));
    Serial.print(temp2);
    Serial.print(F(" "));
    Serial.print(temp3);
    Serial.print(F(" "));
    Serial.print(tempTC);
    Serial.print(F(" "));
    Serial.print(tempRoom);
    Serial.print(F(" "));
    Serial.print(humidRoom);
    Serial.print(F(" "));
    Serial.print(gasZemPlyn);
    Serial.print(F(" "));
    Serial.print(gasCO);
    Serial.print(F(", EtherTimeout = "));
    Serial.print(EtherQueryLast - time);
    Serial.println();

    Measure_TimeAgain = time + Measure_TimePeriod;

  }

  //ETHERNET PART
  if (EthernetClient client = server.available())
  {
    Serial.println(F("server.available()"));
    Serial.print(F("freeRam=")); Serial.println(freeRam());

//    size_t size;
//    Serial.println(F("client.available()"));
//    while ((size = client.available()) > 0)
//    {
//      if (size > 1) {
//        size = 1;
//      }
//      uint8_t* msg = (uint8_t*)malloc(size);
//      size = client.read(msg, size);
//      free(msg);
//    }

    client.print(temp1);
    //client.print(F("."));
    //client.print((int)(temp1 * 10) % 10);
    client.print(F(" "));
    client.print(temp2);
    //client.print(F("."));
    //client.print((int)(temp2 * 10) % 10);
    client.print(F(" "));
    client.print(temp3);
    //client.print(F("."));
    //client.print((int)(temp3 * 10) % 10);
    client.print(F(" "));
    client.print(tempTC);
    client.print(F(" "));
    client.print(tempRoom);
    client.print(F(" "));
    client.print(humidRoom);
    client.print(F(" "));
    client.print(gasZemPlyn);
    client.print(F("."));
    client.print(gasZemPlyn);
    client.print(F(" "));
    client.print(gasCO);
    client.print(F(" "));
    client.print(time);

    if (time < ReceiveSaunaValidTime) {
      client.print(F(" SAUNAOK"));
      client.print(F(" "));
      client.print(saunaskutTemp);
      client.print(F(" "));
      client.print(saunanastTemp);
      client.print(F(" "));
      client.print(saunacasVyp);
      client.print(F(" "));
      client.print(saunaohrevSpirala[1]);
      client.print(F(" "));
      client.print(saunaohrevSpirala[2]);
      client.print(F(" "));
      client.print(saunaohrevSpirala[3]);
    }
    else {
      client.print(F(" SAUNATIMEOUT"));
      client.print(F(" 0 0 0 0 0 0"));
    }

    client.stop();
    EtherQueryLast = time + EtherQueryTimeout;

    Serial.println(F("Eth sent."));
  }


  //reset ak sa ethernet neozyva
  if ( EtherQueryLast < time )
  {
    Serial.println(F("EtherQueryLast < time"));
    while (1);
  }

}







int getTemp1() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte dsdata[12];
  byte dsaddr[8];

  Serial.println(F("getTemp1: "));

  if ( !ds_Temp1.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_Temp1.reset_search();
    Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    Serial.print(F("no device!"));
    return dsRetErrValue;
  }

  ds_Temp1.reset();
  ds_Temp1.select(dsaddr);
  ds_Temp1.write(0x44); //for parasite change to (0x44,1);
  ds_Temp1.reset();
  ds_Temp1.select(dsaddr);
  ds_Temp1.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    dsdata[i] = ds_Temp1.read();
  }

  ds_Temp1.reset_search();

  byte MSB = dsdata[1];
  byte LSB = dsdata[0];

  float dsTemperatureSum = ((MSB << 8) | LSB); //using two's compliment
  dsTemperatureSum = dsTemperatureSum / 16;

  if (dsdata[8] != OneWire::crc8(dsdata, 8)) {
    Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  temp1 = (float)(dsTemperatureSum * 10);
}



int getTemp2() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte dsdata[12];
  byte dsaddr[8];

  Serial.println(F("getTemp2: "));

  if ( !ds_Temp2.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_Temp2.reset_search();
    Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    Serial.print(F("no device!"));
    return dsRetErrValue;
  }

  ds_Temp2.reset();
  ds_Temp2.select(dsaddr);
  ds_Temp2.write(0x44); // start conversion

  ds_Temp2.reset();
  ds_Temp2.select(dsaddr);
  ds_Temp2.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    dsdata[i] = ds_Temp2.read();
  }

  ds_Temp2.reset_search();

  byte MSB = dsdata[1];
  byte LSB = dsdata[0];

  float dsTemperatureSum = ((MSB << 8) | LSB); //using two's compliment
  dsTemperatureSum = dsTemperatureSum / 16;

  if (dsdata[8] != OneWire::crc8(dsdata, 8)) {
    Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  temp2 = (float)(dsTemperatureSum * 10);
}


int getTemp3() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte dsdata[12];
  byte dsaddr[8];

  Serial.println(F("getTemp3: "));

  if ( !ds_Temp3.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_Temp3.reset_search();
    Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    Serial.print(F("no device!"));
    return dsRetErrValue;
  }

  ds_Temp3.reset();
  ds_Temp3.select(dsaddr);
  ds_Temp3.write(0x44); // start conversion

  ds_Temp3.reset();
  ds_Temp3.select(dsaddr);
  ds_Temp3.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    dsdata[i] = ds_Temp3.read();
  }

  ds_Temp3.reset_search();

  byte MSB = dsdata[1];
  byte LSB = dsdata[0];

  float dsTemperatureSum = ((MSB << 8) | LSB); //using two's compliment
  dsTemperatureSum = dsTemperatureSum / 16;

  if (dsdata[8] != OneWire::crc8(dsdata, 8)) {
    Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  temp3 = (float)(dsTemperatureSum * 10);
}



int getTempRoom() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte dsdata[12];
  byte dsaddr[8];

  Serial.println(F("getTempRoom: "));

  if ( !ds_tempRoom.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_tempRoom.reset_search();
    Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    Serial.print(F("no device!"));
    return dsRetErrValue;
  }

  ds_tempRoom.reset();
  ds_tempRoom.select(dsaddr);
  ds_tempRoom.write(0x44); // start conversion

  ds_tempRoom.reset();
  ds_tempRoom.select(dsaddr);
  ds_tempRoom.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    dsdata[i] = ds_tempRoom.read();
  }

  ds_tempRoom.reset_search();

  byte MSB = dsdata[1];
  byte LSB = dsdata[0];

  float dsTemperatureSum = ((MSB << 8) | LSB); //using two's compliment
  dsTemperatureSum = dsTemperatureSum / 16;

  if (dsdata[8] != OneWire::crc8(dsdata, 8)) {
    Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  tempRoom = (float)(dsTemperatureSum * 10);
}




float getgasZemPlyn(void)
{
  float Vcc = readVcc();
  delay(2);
  float volt = analogRead(analoggasZemPlynPin);
  volt = (volt / 1023.0) * Vcc; // only correct if Vcc = 5.0 volts

  return volt;
}


float getgasCO(void)
{
  float Vcc = readVcc();
  delay(2);
  float volt = analogRead(analoggasCOPin);
  volt = (volt / 1023.0) * Vcc; // only correct if Vcc = 5.0 volts

  return volt;
}




float getTC(void)
{
  float Vcc = readVcc();
  delay(2);
  float volt = analogRead(analogTCPin);
  volt = (volt / 1023.0) * Vcc; // only correct if Vcc = 5.0 volts

  return volt;
}



long readVcc(void) {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = 1099560L / result; // Back-calculate AVcc in mV
  return result;
}



void checkReceiveSauna433MHz(void) {
  int i;

  buflen = 12;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    if (buf[0] == 'S') {
      ReceiveSaunaValidTime = time + ReceiveSaunaTimeout;

      saunaskutTemp = (float)buf[1] + (float)buf[2] / 10;
      saunanastTemp = buf[3];
      saunacasVyp = (float)buf[6] * 60 + (float)buf[7];
      saunaohrevSpirala[1] = buf[8];
      saunaohrevSpirala[2] = buf[9];
      saunaohrevSpirala[3] = buf[10];

      Serial.print(F("Sauna 433MHz: "));
      for (i = 0; i < buflen; i++)
      {
        Serial.print(buf[i], DEC);
        Serial.print(F(" "));
      }
      Serial.println();
      Serial.print(F(" "));
      Serial.print(saunaskutTemp);
      Serial.print(F(" "));
      Serial.print(saunanastTemp);
      Serial.print(F(" "));
      Serial.print(saunacasVyp);
      Serial.print(F(" "));
      Serial.print(saunaohrevSpirala[1]);
      Serial.print(F(" "));
      Serial.print(saunaohrevSpirala[2]);
      Serial.print(F(" "));
      Serial.print(saunaohrevSpirala[3]);
      Serial.println();
    }
  }

}




void read_sensors(void)
{
  getTemp1();
  getTemp2();
  getTemp3();
  getTempRoom();

  tempTC = getTC();
  gasZemPlyn = getgasZemPlyn();
  gasCO = getgasCO();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(t) || isnan(h)) {
    Serial.println(F("DHT: failed"));
  }
  else {
    humidRoom = h;
  }
}




int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}





































































