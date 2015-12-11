#include <OneWire.h>
#include <avr/wdt.h>

#include <VirtualWire.h> //433MHz

#include "etherShield.h"
#include "ETHER_28J60.h"

#include "DHT.h"

#define DHTPIN 2 // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)

static uint8_t mac[6] = {
  0x74,0x69,0x69,0x2D,0x30,0x31 };   
static uint8_t ip[4] = {
  192, 168, 1, 202};                       

static uint16_t port = 80;                                      // Use port 80 - the standard for HTTP
ETHER_28J60 ethernet;

//433MhZ
const int receive433MHz_pin = 9; //povodne 11
uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;

int DS18S20_Pin_Temp1 = 5; //DS18S20 Signal pin 
int DS18S20_Pin_Temp2 = 6; //DS18S20 Signal pin 
int DS18S20_Pin_Temp3 = 7; //DS18S20 Signal pin 
int DS18S20_Pin_tempRoom = 4; //DS18S20 Signal pin 

float temp1;
float temp2;
float temp3;
float tempTC;
float tempRoom;
int humidRoom;
float gasZemPlyn;
float gasCO;


//TC
const int analogTCPin = A4;  
const float TC_m = 0.10371769557;
const float TC_b = 37.3700094445;
const int TC_repeat = 100;
float TC_val, TCValue = 0; 

const int analoggasZemPlynPin = A0;  
const int analoggasCOPin = A1;  

float gasZemPlyn_val, gasZemPlynValue = 0; 
float gasCO_val, gasCOValue = 0; 

unsigned long time;
const unsigned long Measure_TimePeriod = 60 * 1000L; //repeat reading from sensors
unsigned long Measure_TimeAgain = 0;

const long EtherQueryTimeout = 10 * 60 * 1000L;
unsigned long EtherQueryLast;

const long ReceiveSaunaTimeout = 1 * 60 * 1000L;
unsigned long ReceiveSaunaValidTime;

//sauna
float saunaskutTemp, saunanastTemp;
long saunacasVyp;
int saunaohrevSpirala[5]={
  0,0,0,0};

unsigned long testcounter;

//Temperature chip i/o
OneWire ds_Temp1(DS18S20_Pin_Temp1); 
OneWire ds_Temp2(DS18S20_Pin_Temp2); 
OneWire ds_Temp3(DS18S20_Pin_Temp3);
OneWire ds_tempRoom(DS18S20_Pin_tempRoom);

DHT dht(DHTPIN, DHTTYPE);


void setup(){ 
  wdt_enable(WDTO_8S);

  time = millis();

  Serial.begin(115200);

  Serial.println(); 
  Serial.println(); 
  Serial.print(F("KotolEthernet is starting "));   

  pinMode(analogTCPin, INPUT);

  dht.begin();

  vw_set_rx_pin(receive433MHz_pin);
  vw_setup(2000);	 // Bits per sec
  vw_rx_start();       // Start the receiver PLL running

  ethernet.setup(mac, ip, port);
  EtherQueryLast = time + EtherQueryTimeout;

  wdt_reset(); //wait for ds18b20
  delay(2000);
  read_sensors();

  Serial.println(F("go!")); 
}

void loop(){
  wdt_reset();

  time = millis();

  //  long li = (Measure_TimeAgain - time)/1000;
  //  li = li % 100;
  //  lcd.setCursor(14,0);
  //  lcd.print(li);



  checkReceiveSauna433MHz();

  if(time > Measure_TimeAgain)  
  {
    read_sensors();
    Serial.println(F("Measured."));
    Serial.print(temp1);  
    Serial.print(" ");
    Serial.print(temp2);  
    Serial.print(" ");
    Serial.print(temp3);  
    Serial.print(" ");
    Serial.print(tempTC);  
    Serial.print(" ");
    Serial.print(tempRoom);  
    Serial.print(" ");
    Serial.print(humidRoom);  
    Serial.print(" ");   
    Serial.print(gasZemPlyn);  
    Serial.print(" ");
    Serial.print(gasCO);  
    Serial.print(F(", EtherTimeout = "));
    long tmp = (EtherQueryLast - time) / 1000;
    Serial.print(tmp);  
    Serial.println();  

    Measure_TimeAgain = time + Measure_TimePeriod;

  }

  //ETHERNET PART        
  if (ethernet.serviceRequest())
  {
    ethernet.print(temp1);
    ethernet.print(".");
    ethernet.print((int)(temp1*10) % 10);
    ethernet.print(" ");
    ethernet.print(temp2);
    ethernet.print(".");
    ethernet.print((int)(temp2*10) % 10);
    ethernet.print(" ");
    ethernet.print(temp3);
    ethernet.print(".");
    ethernet.print((int)(temp3*10) % 10);
    ethernet.print(" ");
    ethernet.print(tempTC);
    ethernet.print(" ");
    ethernet.print(tempRoom);
    ethernet.print(".");
    ethernet.print((int)(tempRoom*10) % 10);
    ethernet.print(" ");
    ethernet.print(humidRoom);
    ethernet.print(" ");
    ethernet.print(gasZemPlyn);
    ethernet.print(".");
    ethernet.print((int)(gasZemPlyn*100) % 100);
    ethernet.print(" ");
    ethernet.print(gasCO);
    ethernet.print(".");
    ethernet.print((int)(gasCO*100) % 100);
    ethernet.print(" ");
    testcounter = time / 1000; 
    ethernet.print(testcounter);

    if(time < ReceiveSaunaValidTime){
      ethernet.print(" SAUNAOK");  
      ethernet.print(" ");
      ethernet.print(saunaskutTemp);
      ethernet.print(".");
      ethernet.print((int)(saunaskutTemp*10) % 10);
      ethernet.print(" ");
      ethernet.print(saunanastTemp);
      ethernet.print(" ");
      ethernet.print(saunacasVyp);
      ethernet.print(" ");
      ethernet.print(saunaohrevSpirala[1]);
      ethernet.print(" ");
      ethernet.print(saunaohrevSpirala[2]);
      ethernet.print(" ");
      ethernet.print(saunaohrevSpirala[3]);
    }
    else{
      ethernet.print(" SAUNATIMEOUT");  
      ethernet.print(" 0.0 0 0 0 0 0");  
    }


    ethernet.respond();
    EtherQueryLast = time + EtherQueryTimeout;

    Serial.println(F("Ethernet sent."));  
    //    Serial.print(temp1); 
    //    Serial.print(" ");
    //    Serial.print(temp2); 
    //    Serial.print(" ");
    //    Serial.print(temp3); 
    //    Serial.print(" ");
    //    Serial.print(tempTC);     
    //    Serial.print(" ");
    //    Serial.print(tempRoom);     
    //    Serial.print(" ");
    //    Serial.print(humidRoom);     
    //    Serial.print(" ");
    //    Serial.print(gasZemPlyn); 
    //    Serial.print(" ");
    //    Serial.println(gasCO);     

  }


  //reset ak sa ethernet neozyva
  if( EtherQueryLast < time ) 
  {
    while(1);
  }

}







float getTemp1(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_Temp1.search(addr)) {
    //no more sensors on chain, reset search
    ds_Temp1.reset_search();
    Serial.println(F("getTemp1: "));
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

  ds_Temp1.reset();
  ds_Temp1.select(addr);
  ds_Temp1.write(0x44); //for parasite change to (0x44,1);   
  ds_Temp1.reset();
  ds_Temp1.select(addr);
  ds_Temp1.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_Temp1.read();
  }

  ds_Temp1.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print("ERROR: CRC didn't match\n");
    return -1000;
  }

  temp1 = TemperatureSum;

  return TemperatureSum;
}



float getTemp2(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_Temp2.search(addr)) {
    //no more sensors on chain, reset search
    ds_Temp2.reset_search();
    Serial.println(F("getTemp2: "));
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

  ds_Temp2.reset();
  ds_Temp2.select(addr);
  ds_Temp2.write(0x44); // start conversion

  ds_Temp2.reset();
  ds_Temp2.select(addr);
  ds_Temp2.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_Temp2.read();
  }

  ds_Temp2.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print("ERROR: CRC didn't match\n");
    return -1000;
  }

  temp2 = TemperatureSum;

  return TemperatureSum;
}


float getTemp3(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_Temp3.search(addr)) {
    //no more sensors on chain, reset search
    ds_Temp3.reset_search();
    Serial.println(F("getTemp3: "));
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

  ds_Temp3.reset();
  ds_Temp3.select(addr);
  ds_Temp3.write(0x44); // start conversion

  ds_Temp3.reset();
  ds_Temp3.select(addr);
  ds_Temp3.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_Temp3.read();
  }

  ds_Temp3.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print("ERROR: CRC didn't match\n");
    return -1000;
  }

  temp3 = TemperatureSum;

  return TemperatureSum;
}



float getTempRoom(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_tempRoom.search(addr)) {
    //no more sensors on chain, reset search
    ds_tempRoom.reset_search();
    Serial.println(F("getTempRoom: "));
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

  ds_tempRoom.reset();
  ds_tempRoom.select(addr);
  ds_tempRoom.write(0x44); // start conversion

  ds_tempRoom.reset();
  ds_tempRoom.select(addr);
  ds_tempRoom.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_tempRoom.read();
  }

  ds_tempRoom.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print("ERROR: CRC didn't match\n");
    return -1000;
  }

  tempRoom = TemperatureSum;

  return TemperatureSum;
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
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1099560L / result; // Back-calculate AVcc in mV
  return result;
}



void checkReceiveSauna433MHz(void){

  int i;

  buflen = 12;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    if(buf[0]=='S'){
      ReceiveSaunaValidTime = time + ReceiveSaunaTimeout;

      saunaskutTemp = (float)buf[1] + (float)buf[2] / 10;  
      saunanastTemp = buf[3];
      saunacasVyp = (float)buf[6]*60 + (float)buf[7];
      saunaohrevSpirala[1] = buf[8];
      saunaohrevSpirala[2] = buf[9];
      saunaohrevSpirala[3] = buf[10];

      //      Serial.print("Sauna 433MHz: ");	
      //      for (i = 0; i < buflen; i++)
      //      {
      //        Serial.print(buf[i], DEC);
      //        Serial.print(" ");
      //      }
      //      Serial.println();
      //
      //      Serial.print(" ");
      //      Serial.print(saunaskutTemp);  
      //      Serial.print(" ");
      //      Serial.print(saunanastTemp);  
      //      Serial.print(" ");
      //      Serial.print(saunacasVyp);  
      //      Serial.print(" ");
      //      Serial.print(saunaohrevSpirala[1]);  
      //      Serial.print(" ");
      //      Serial.print(saunaohrevSpirala[2]);  
      //      Serial.print(" ");
      //      Serial.print(saunaohrevSpirala[3]);
      //      Serial.println();  
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
    Serial.println(F("Failed to read from DHT"));
  } 
  else {
    humidRoom = h;
  }
}








































































