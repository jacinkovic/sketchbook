#include <OneWire.h>
#include <avr/wdt.h>

#include <VirtualWire.h> //433MHz

#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

#include "DHT.h"

#define DEBUG

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
const byte DS18S20_Pin_TempRoom = 4; //DS18S20 Signal pin

//Temperature chip i/o
OneWire ds_Temp1(DS18S20_Pin_Temp1);
OneWire ds_Temp2(DS18S20_Pin_Temp2);
OneWire ds_Temp3(DS18S20_Pin_Temp3);
OneWire ds_tempRoom(DS18S20_Pin_TempRoom);

int temp1;
int temp2;
int temp3;
int tempTC;
int tempRoom;
int humidRoom;
int gasZemPlyn;
int gasCO;


//TC
//const float TC_m = 0.10371769557;
//const float TC_b = 37.3700094445;

const byte analogTCPin = A5;
const byte TC_repeat = 100;
const byte analoggasZemPlynPin = A0;
const byte analoggasCOPin = A1;

const unsigned long MeasureTime_Period = 60 * 1000L;
unsigned long MeasureTimeLast;

const unsigned long DebugTime_Period = 1000L;
unsigned long DebugTimeLast;

const unsigned long EtherQuery_Timeout = 15 * 60000L;
unsigned long EtherQueryLast;

const unsigned long ReceiveSauna_Timeout = 2 * 60000L;
unsigned long ReceiveSaunaLast;

//sauna
int saunaskutTemp, saunanastTemp;
int saunacasVyp;
byte saunaohrevSpirala;





void setup() {
  wdt_enable(WDTO_8S);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(F("setup();"));
#endif

  pinMode(analogTCPin, INPUT);
  dht.begin();

  for (int i = 0; i < 10; i++) {
    wdt_reset();
    delay(500);
    read_sensors();
  }

  vw_set_rx_pin(receive433MHz_pin);
  vw_setup(2000);	 // Bits per sec
  vw_rx_start();   // Start the receiver PLL running

  ReceiveSaunaLast = millis() - ReceiveSauna_Timeout;

  uint8_t mac[6] = {0x74, 0x73, 0x71, 0x2D, 0x32, 0x32};
  IPAddress myIP(192, 168, 1, 202);
  Ethernet.begin(mac, myIP);
  server.begin();

#ifdef DEBUG
  Serial.println(F("loop();"));
#endif

}

void loop()
{
  wdt_reset();

  checkReceiveSauna433MHz();

  if ((long)(millis() - MeasureTimeLast > MeasureTime_Period))
  {
#ifdef DEBUG
  Serial.println(F("measuring time: "));
#endif
    read_sensors();
    MeasureTimeLast = millis();
  }

  checkEth();

  //reset ak sa ethernet neozyva
  if ((long)(millis() - EtherQueryLast > EtherQuery_Timeout))
  {
#ifdef DEBUG
    Serial.println(F("EtherQueryLastTimeout"));
#endif
    while (1);
  }

#ifdef DEBUG
  if ((long)(millis() - DebugTimeLast > DebugTime_Period))
  {
    Serial.print(F("."));
    DebugTimeLast = millis();
  }
#endif

}







int getTemp1() {
  //returns the temperature from one DS18S20 in DEG Celsius

  const int dsRetErrValue = -1000;
  byte dsdata[12];
  byte dsaddr[8];

#ifdef DEBUG
  Serial.println(F("getTemp1."));
#endif

  if ( !ds_Temp1.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_Temp1.reset_search();
    //Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    //Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    //Serial.print(F("no device!"));
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
    //Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  temp1 = (float)(dsTemperatureSum * 10);
}



int getTemp2() {
  //returns the temperature from one DS18S20 in DEG Celsius

  const int dsRetErrValue = -1000;
  byte dsdata[12];
  byte dsaddr[8];

#ifdef DEBUG
  Serial.println(F("getTemp2."));
#endif

  if ( !ds_Temp2.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_Temp2.reset_search();
    //Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    //Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    //Serial.print(F("no device!"));
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
    //Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  temp2 = (float)(dsTemperatureSum * 10);
}


int getTemp3() {
  //returns the temperature from one DS18S20 in DEG Celsius

  const int dsRetErrValue = -1000;
  byte dsdata[12];
  byte dsaddr[8];

#ifdef DEBUG
  Serial.println(F("getTemp3."));
#endif

  if ( !ds_Temp3.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_Temp3.reset_search();
    //Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    //Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    //Serial.print(F("no device!"));
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
    //Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  temp3 = (float)(dsTemperatureSum * 10);
}



int getTempRoom() {
  //returns the temperature from one DS18S20 in DEG Celsius

  const int dsRetErrValue = -1000;
  byte dsdata[12];
  byte dsaddr[8];

#ifdef DEBUG
  Serial.println(F("getTempRoom."));
#endif

  if ( !ds_tempRoom.search(dsaddr)) {
    //no more sensors on chain, reset search
    ds_tempRoom.reset_search();
    //Serial.println(F("no sensor!"));
    return dsRetErrValue;
  }

  if ( OneWire::crc8( dsaddr, 7) != dsaddr[7]) {
    //Serial.println(F("CRC invalid!"));
    return dsRetErrValue;
  }

  if ( dsaddr[0] != 0x10 && dsaddr[0] != 0x28) {
    //Serial.print(F("no device!"));
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
    //Serial.print(F("CRC invalid!_"));
    return dsRetErrValue;
  }

  tempRoom = (float)(dsTemperatureSum * 10);
}




float getgasZemPlyn(void)
{
#ifdef DEBUG
  Serial.println(F("getgasZemPlyn."));
#endif

  float Vcc = readVcc();
  delay(2);
  float volt = analogRead(analoggasZemPlynPin);
  volt = (volt / 1023.0) * Vcc; // only correct if Vcc = 5.0 volts

  return volt;
}


float getgasCO(void)
{
#ifdef DEBUG
  Serial.println(F("getgasCO."));
#endif
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

      ReceiveSaunaLast = millis();

      saunaskutTemp = (float)buf[1] + (float)buf[2] / 10;
      saunanastTemp = buf[3];
      saunacasVyp = (float)buf[6] * 60 + (float)buf[7];
      saunaohrevSpirala = buf[8] + buf[9] + buf[10];

#ifdef DEBUG
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
      Serial.print(saunaohrevSpirala);
      Serial.println();
#endif
    }

  }

  wdt_reset();
}


void checkEth(void) {

  //ETHERNET PART
  if (EthernetClient client = server.available())
  {
#ifdef DEBUG
    Serial.println(F("eth."));
#endif

    client.print(temp1);
    client.print(F(" "));
    client.print(temp2);
    client.print(F(" "));
    client.print(temp3);
    client.print(F(" "));
    client.print(tempTC);
    client.print(F(" "));
    client.print(tempRoom);
    client.print(F(" "));
    client.print(humidRoom);
    client.print(F(" "));
    client.print(gasZemPlyn);
    client.print(F(" "));
    client.print(gasCO);
    client.print(F(" "));
    client.print((millis() / 1000));

    if ((long)(millis() - ReceiveSaunaLast < ReceiveSauna_Timeout)) {

      client.print(F(" SAOK"));
      client.print(F(" "));
      client.print(saunaskutTemp);
      client.print(F(" "));
      client.print(saunanastTemp);
      client.print(F(" "));
      client.print(saunacasVyp);
      client.print(F(" "));
      client.print(saunaohrevSpirala);

    }
    else {
      client.print(F(" SAOFF"));
      client.print(F(" 0 0 0 0"));
      ReceiveSaunaLast = millis() - ReceiveSauna_Timeout; //update timeout to overflow
    }

    client.stop();
    EtherQueryLast = millis();
  }

  wdt_reset();
}


void read_sensors(void)
{
#ifdef DEBUG
    Serial.println(F("read_sensors: "));
#endif
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
#ifdef DEBUG
    Serial.println(F("DHT: failed"));
#endif
  }
  else {
    humidRoom = h;
  }

#ifdef DEBUG
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
  Serial.println();
#endif
}



//int freeRam()
//{
//  extern int __heap_start, *__brkval;
//  int v;
//  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
//}





































































