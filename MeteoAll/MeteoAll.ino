#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include <EtherCard.h>   //old library, but works with Mirf
#include "DHT.h"

#define   nobits   40//36
#define   smin   7500L
#define   smax   9900L
#define   semin   250L
#define   semax   750L
#define   lmin   1700L
#define   lmax   2300L
#define   hmin   3700L
#define   hmax   4300L
#define ReceiverPin 2

const int VBatPin = A0;
const int TempOut1_DS18S20_Pin = 4;
const int TempOut2_DS18S20_Pin = 5;

const unsigned long Rec_Timeout_Limit = 15 * 60000L;  //timeout for messages from Auriol
const unsigned long Rec_Timeout_StartValue = 99999999L; //to get timeout on start

unsigned long rec_time_TempHumid = Rec_Timeout_StartValue;
unsigned long rec_time_WindAverage = Rec_Timeout_StartValue;
unsigned long rec_time_WindDirectionGust = Rec_Timeout_StartValue;
unsigned long rec_time_Rain = Rec_Timeout_StartValue;

int recTimeoutStat; 

char chksum_raingauge[4], chksum_combsensor[4];
char code[40];

int ID;
int batt_combsensor=-1, batt_raingauge=-1;  //initial value as not functional
int windButton;
int tempOutButton;
float tempOut;
int humidOut;
float windGust;
int windDir;
float windAvg;
float rain;
float rain_initialshift = -1;
float VBat;
float TempOut1, TempOut2;

OneWire ds_TempOut1(TempOut1_DS18S20_Pin);
OneWire ds_TempOut2(TempOut2_DS18S20_Pin);
unsigned long time;



int i;

const unsigned long wdt_TimePeriod = 1 *1000L; //1 sec in ms
unsigned long wdt_TimeAgain = 0;
const unsigned long common_TimePeriod = 60 *1000L; //1 sec in ms
unsigned long common_TimeAgain = 0;


int windDirAvg= -1;
const int windDirAvg_Deg = 5;  //avg step in degrees

// ethernet interface mac address
static byte mymac[] = { 
  0x74,0x69,0x69,0x2D,0x30,0x32 };
// ethernet interface ip address
static byte myip[] = { 
  192,168,1,201 };
// gateway ip address
static byte gwip[] = { 
  192,168,1,1 };

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer
BufferFiller bfill;
char buff[12];

const byte DHTPIN = 7; // what pin we're connected to
#define DHTTYPE DHT22 // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

int DHTTemp;
int DHTHumid;

int ethCounter;
int runtimeCounter;

const long EtherQueryTimeout = 10 * 60 * 1000L;
unsigned long EtherQueryLast;



#define BMP085_ADDRESS 0x77  // I2C address of BMP085
const unsigned char OSS = 3;  // Oversampling Setting
const long BaseAltitudeCoef = 200 *0.12;  //0.12 hPa for every 100 meters



// Calibration values
int ac1;
int ac2; 
int ac3; 
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1; 
int b2;
int mb;
int mc;
int md;

// b5 is calculated in bmp085GetTemperature(...)
// this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 

float bmp085temperature;
float bmp085pressure;


const int buzzer = 9; //buzzer to arduino pin 9

void setup(){

  wdt_enable(WDTO_8S);

  pinMode(buzzer, OUTPUT); // Set buzzer 

  time = millis();

  Serial.begin(115200);

  Serial.println(); 
  Serial.print(F("Code "));
  Serial.print(F(__FILE__));   
  Serial.print(F(" built at: "));   
  Serial.print(F(__DATE__)); 
  Serial.print(F(" "));   
  Serial.println(F(__TIME__));
  Serial.println(F("setup();"));

  //bmp085
  Wire.begin();
  bmp085Calibration();

  dht.begin();

  ether.begin(sizeof Ethernet::buffer, mymac);
  ether.staticSetup(myip);

  pinMode(VBatPin, INPUT);
  pinMode(ReceiverPin, INPUT);

  EtherQueryLast = time + EtherQueryTimeout;

  //first read to prepare sensors
  getTempOut1();
  getTempOut2();
  //end of first read


  for (int i=0; i<3; i++){
    wdt_reset();
    tone(buzzer, 2000); // Send 1KHz sound signal...
    delay(300);
    tone(buzzer, 1000); // Send 1KHz sound signal...
    delay(300);
  }
  noTone(buzzer);     // Stop sound...

  Serial.println(F("loop();"));   

}

void loop(){
  time = millis();

  //read weatherstation radio signal
  int ret_code = RCode();

  //decode weatherstation radio signal
  if(ret_code == 0){
    tone(buzzer, 5000);
    //SerialCode();
    getGeneral();
    getRain();
    getWindDirectionGust();
    getWindAverage();
    getTempHumid();
    noTone(buzzer);    
  }




  //ETHERNET PART        
  CheckEth();


  //check other sensors
  if(time > common_TimeAgain)  //watchdog
  {
    //tone(buzzer, 5000);
    common_TimeAgain = time + common_TimePeriod;

    //bmp085
    bmp085temperature = (float)(bmp085GetTemperature(bmp085ReadUT()))/10;
    bmp085pressure = (float)bmp085GetPressure(bmp085ReadUP())/100 + BaseAltitudeCoef; //hpa
    if((bmp085pressure<900) || (bmp085pressure>1100)){ 
      Serial.println(F("Reset reason: (bmp085pressure<900) || (bmp085pressure>1100)"));
      while(1); //error check, if problem then restart
    }
    //getVBat();
    getTempOut1();
    getTempOut2();
    getDHT();
    Set_windDirAvg();
    runtimeCounter++;
    getRecTimeoutStat();
    SerialAll();
  }

  //watchdog
  if(time > wdt_TimeAgain)  //watchdog
  {
    wdt_TimeAgain = time + wdt_TimePeriod;
    wdt_reset();
    Serial.println(F("."));


    //reset vzdy po nastavenom case
    if( EtherQueryLast < time )
    {
      Serial.println(F("Reset reason: EtherQueryLast < time"));
      while(1); //no Ethernet request for a long time, probably problem with communication, reset
    }


  }


}




// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1  = bmp085ReadInt(0xB6);
  b2  = bmp085ReadInt(0xB8);
  mb  = bmp085ReadInt(0xBA);
  mc  = bmp085ReadInt(0xBC);
  md  = bmp085ReadInt(0xBE);
}

// Calculate temperature given ut.
// Value returned will be in units of 0.1 deg C
short bmp085GetTemperature(unsigned int ut)
{
  long x1, x2;
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;
  return ((b5 + 8)>>4);  
}

// Calculate pressure 
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;

  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;

  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;

  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;

  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  return p;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available())
    ;
  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;

  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT()
{
  unsigned int ut;

  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();

  // Wait at least 4.5ms
  delay(5);

  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;

  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();

  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));

  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom(BMP085_ADDRESS, 3);

  // Wait for data to become available
  while(Wire.available() < 3)
    ;
  msb  = Wire.read();
  lsb  = Wire.read();
  xlsb = Wire.read();
  up=(((unsigned long)msb<<16)|((unsigned long)lsb<<8)|(unsigned long)xlsb)>>(8-OSS);
  return up;
}






//void addLongTobfill(long in)
//{
//  ltoa(in, buff, 10);
//  bfill.emit_raw(buff, strlen(buff));
//  bfill.emit_p(PSTR(" "));
//  Serial.print(buff);
//  Serial.print(" ");
//}




void addFloatTobfill(float in)
{
  Serial.print(in);
  Serial.print(F(" "));

  in = (floor(in*100))/100 ;
  long in_int = abs(in);
  if(in<0){ 
    bfill.emit_p(PSTR("-")); 
  } 
  long in_des = abs((float)in*100) - in_int*100;

  ltoa(in_int, buff, 10);
  bfill.emit_raw(buff, strlen(buff));
  bfill.emit_p(PSTR("."));
  ltoa(in_des, buff, 10);
  bfill.emit_raw(buff, strlen(buff));
  bfill.emit_p(PSTR(" "));     
}




void SerialAll(void)
{    
  Serial.print(F("tempOut="));
  Serial.print(tempOut);
  Serial.print(F(" humidOut="));
  Serial.print(humidOut);

  Serial.print(F(" windDir="));
  Serial.print(windDir);
  Serial.print(F(" windGust=")); 
  Serial.print(windGust);
  Serial.print(F(" windAvg="));
  Serial.println(windAvg);

  Serial.print(F(" rain="));
  Serial.print(rain);

  Serial.print(F(" batt_combsensor="));
  Serial.print(batt_combsensor);
  Serial.print(F(", batt_raingauge="));
  Serial.println(batt_raingauge);

  Serial.print(F(" windDirAvg="));
  Serial.print(windDirAvg);  

  Serial.print(F(" bmp085temperature="));
  Serial.print(bmp085temperature);
  Serial.print(F(" bmp085pressure="));
  Serial.println(bmp085pressure);

  Serial.print(F(" TempOut1="));
  Serial.print(TempOut1);
  Serial.print(F(" TempOut2="));
  Serial.print(TempOut2);

  Serial.print(F(" DHTTemp="));
  Serial.print(DHTTemp);
  Serial.print(F(" DHTHumid="));
  Serial.print(DHTHumid);

  Serial.print(F(" runtimeCounter="));
  Serial.println(runtimeCounter);
  
  
//  Serial.print(F(" recTimeoutStat="));
//  Serial.println(recTimeoutStat);  
//
//  Serial.print(F(" EtherTimeout="));
//  long tmp = (EtherQueryLast - time) / 1000;
//  Serial.print(tmp);  
//
//  Serial.print(F(" ethCounter="));
//  Serial.println(ethCounter);  

//  Serial.print(F("available memory="));
//  Serial.println(freeRam(), DEC);

} 







void Set_windDirAvg(void)
{
  if(windDirAvg == -1){ //first read
    windDirAvg = windDir; 
    return;
  }

  if(windDirAvg == windDir){ //no change in direction
    return;
  }



  int windDirAvg_tmp;
  windDirAvg_tmp = windDir - windDirAvg;
  if( (windDir - windDirAvg) > (windDirAvg - windDir) ){ //first read
    if(abs(windDirAvg_tmp) <= 180){
      windDirAvg = windDirAvg + windDirAvg_Deg; 
    }
    else{
      windDirAvg = windDirAvg - windDirAvg_Deg;
    }
  }
  else{
    if(abs(windDirAvg_tmp) <= 180){
      windDirAvg = windDirAvg - windDirAvg_Deg; 
    }
    else{
      windDirAvg = windDirAvg + windDirAvg_Deg;
    } 
  }

  if(windDirAvg < 0){ 
    windDirAvg = windDirAvg + 360; 
  }
  if(windDirAvg >= 360){ 
    windDirAvg = windDirAvg - 360; 
  }

  //           0
  //      315      45
  //    270          90
  //      225     135
  //          180

}





float buildUpFloat(long outbox3, long  outbox2, long  outbox1, long outbox0)
{  
  long output_l =  (outbox3 << 24) | (outbox2 << 16) | (outbox1 << 8) | (outbox0); 
  float output_f = output_l;
  output_f = output_f / 100;
  return output_f;
}



void getRecTimeoutStat(void)  //check timeout for received data from Auriol
{
  recTimeoutStat = 15;

  if ((long)(millis() - rec_time_TempHumid < Rec_Timeout_Limit)){
    recTimeoutStat = recTimeoutStat - 8;
  }
  if ((long)(millis() - rec_time_WindAverage < Rec_Timeout_Limit)){
    recTimeoutStat = recTimeoutStat - 4;
  }
  if ((long)(millis() - rec_time_WindDirectionGust < Rec_Timeout_Limit)){
    recTimeoutStat = recTimeoutStat - 2;
  }
  if ((long)(millis() - rec_time_Rain < Rec_Timeout_Limit)){
    recTimeoutStat = recTimeoutStat - 1;
  }

}



int RCode()

{

  int i, ii;
  long startMicros, endMicros;

  //String code = "001000000110110010101101000000000010"; //rain sensor
  //String code = "xxx110101010110111011100001101000001";  //wind direction + gust
  //String code = "xxx110101010110111011100001101000001";  //

  //code="001000000110110000000011000000000000"; //rain 48.00mm
  //      iiiiiiiivXXb 
  //code="xxx110101010110111000000000000000001"; //N -   0, 0gust
  //code="xxx110101010110111010110100000000000"; //E -  90, 0gust
  //code="xxx110101010110111001011010000000000"; //S - 180, 0gust  
  //code="xxx110101010110111011100001000000000"; //W - 270, 0gust

  //code="110101010010001110110000111001100000"; //22.0C 67%
  //code="110101010010101001110000101001100001"; //22.9C 65%

  //code="110101010110100000000000000000001100" //wavg=0
  //code="110101010110100000000000011000001011" //wavg=1.2 

  //  code = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  for(i=0; i<40; i++){ 
    code[i] = 'x';
  }
  
  if (digitalRead(ReceiverPin)) return 1;

  startMicros = micros();
  while(!digitalRead(ReceiverPin))
  {
    if ((micros()-startMicros)>smax){
      return 2;
    }
  }

  if ((micros()-startMicros)<smin){
    return 3;
  }

  //Serial.print(F("after3 "));

  startMicros = micros();
  while(digitalRead(ReceiverPin))
  {
    if ((micros()-startMicros)>semax){
      return 4;
    }
  }

  if ((micros()-startMicros)<semin){ 
    return 5;
  }

  for(i = 0; i < nobits; i++)
  {
    startMicros = micros();
    while(!digitalRead(ReceiverPin))
    { 
      if ((micros()-startMicros)>smax){ //protection against deadlock
        return 6;
      }
    }

    endMicros = micros();
    if(((endMicros-startMicros)>lmin)&&((endMicros-startMicros)<lmax)){
      code[i]='0';
    }
    else
    {
      if(((endMicros-startMicros)>hmin)&&((endMicros-startMicros)<hmax)){
        code[i]='1';
        //Serial.print(F("&"));
      }
    }

    startMicros = micros();
    while(digitalRead(ReceiverPin))
    {
      if ((micros()-startMicros)>semax){
        return 7;
      }
    }

    if ((micros()-startMicros)<semin){
      return 8;
    }

  }

  //SerialCode();

  //code = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  //code = "101010101010100101001000010001000100xxxx";

  //remove x chars from the beginning - if exist
  //Serial.print(F("removex "));
  for(int a=0; a<3; a++){
    //Serial.println(code[1));
    if(code[0]=='x'){
      for(int b=0; b<nobits-1; b++){ 
        code[b] = code[b+1];
      }
    }  
  }

  //SerialCode();

  //check for x inside the code
  //Serial.print(F("checkx "));
  for(int b=0; b<36; b++){ 
    if(code[b] == 'x'){
      //Serial.println(F(" ERR >>> x in code <<<"));
      return 9;
    }
  }

//  Serial.print(F("Code="));
//  for(i=0; i<40; i++){ 
//    Serial.print(code[i]);
//  }
//  Serial.println();
//  
  get_checksum();

  return 0;
}



//*****************************************************
int bcd_conv_dec(char n0, char n1, char n2, char n3)
{
  int ret = 
    (n0=='1'?1:0) * 1 +
    (n1=='1'?1:0) * 2 +
    (n2=='1'?1:0) * 4 +
    (n3=='1'?1:0) * 8;

  return ret;
}





//*****************************************************
int get_checksum(void)
{

  //chksum
  int n[8], i;
  int na_combsensor = 15; //0xF
  int na_raingauge = 7; //0x7

    //Serial.println(F(" "));
  for(i=0; i<8; i++){
    n[i] = bcd_conv_dec(code[i*4+0], code[i*4+1], code[i*4+2], code[i*4+3]);  
    na_combsensor = na_combsensor - n[i];
    na_raingauge = na_raingauge + n[i];
    //Serial.print(n[i]); 
    //Serial.print(F("    ")); 
  }

  //Serial.println(F(" "));
  //Serial.print(F(" na_combsensor= ")); 
  //Serial.print(na_combsensor); 
  //Serial.print(F("; ")); 
  bcd_conv_bin(na_combsensor, 1); //combsensor

  //Serial.print(F(" na_raingauge= ")); 
  //Serial.print(na_raingauge); 
  //Serial.print(F("; ")); 
  bcd_conv_bin(na_raingauge, 2);  //raingauge

  return 0;
}



//*****************************************************
void bcd_conv_bin(int in, int ver)
{
  int i;
  char chksum[4] = {'0','0','0','0'};

  //negative values
  while(in<=-16){ 
    in = in + 16; 
  }    
  //two's complement
  if(in<0){
    in = 16 + in; 
  } 


  //positive values
  if(in>=0){

    while(in>=16){ 
      in = in - 16; 
    }

    if (in>=8){ 
      chksum[3]='1'; 
      in = in - 8;
    } 
    if (in>=4){ 
      chksum[2]='1';
      in = in - 4;
    } 
    if (in>=2){ 
      chksum[1]='1'; 
      in = in - 2;
    } 
    if (in>=1){ 
      chksum[0]='1';
      in = in - 1;
    } 
  }


  //two type of sensors
  if(ver==1){ 
    //Serial.print(F("chksum_combsens= "));
    for(i=0; i<4; i++){ 
      chksum_combsensor[i] = chksum[i]; 
    }
  }
  if(ver==2){ 
    //Serial.print(F("chksum_raingauge= "));
    for(i=0; i<4; i++){ 
      chksum_raingauge[i] = chksum[i]; 
    }
  }
  //Serial.print(chksum);
  //Serial.println(F("; "));


}


int check_checksum(char in[])
{
  //checksum
  if
    ((code[32]==in[0])&&
    (code[33]==in[1])&&
    (code[34]==in[2])&&
    (code[35]==in[3]))
  { 
    //Serial.println(F("  Checksum OK "));
    return 1;
  }
  else
  { 
    Serial.println(F("  Checksum ERR "));
    return 4;
  }
}



//************************************************************
void getGeneral(void)
{
  //ID
  ID = 
    (code[7]=='1'?1:0)*128 +
    (code[6]=='1'?1:0)*64 +
    (code[5]=='1'?1:0)*32 +
    (code[4]=='1'?1:0)*16 +
    (code[3]=='1'?1:0)*8 +
    (code[2]=='1'?1:0)*4 +
    (code[1]=='1'?1:0)*2 +
    (code[0]=='1'?1:0)*1;
    
    //Serial.println(F("Received: General"));
    //Serial.print(F(" ID="));
    //Serial.println(ID);

    //Serial.print(F(" batt="));
    //if(batt==0) Serial.println(F("OK")); 
    //else Serial.println(F("LOW"));
}



//************************************************************
void getTempHumid(void)
{
  int tempOut_tmp;

  if 
    ((code[9]!='1')||
    (code[10]!='1'))  
  {
    Serial.println(F("Received: TempHumid"));
    if(check_checksum(chksum_combsensor)==1)
    { 
      int tempOut_tmp= 
        (code[23]=='1'?1:0)*2048 +
        (code[22]=='1'?1:0)*1024 +
        (code[21]=='1'?1:0)*512 +
        (code[20]=='1'?1:0)*256 +
        (code[19]=='1'?1:0)*128 +
        (code[18]=='1'?1:0)*64 +
        (code[17]=='1'?1:0)*32 +
        (code[16]=='1'?1:0)*16 +
        (code[15]=='1'?1:0)*8 +
        (code[14]=='1'?1:0)*4 +
        (code[13]=='1'?1:0)*2 +
        (code[12]=='1'?1:0)*1;

      // Calculate negative temperature
      if((tempOut_tmp & 0x800) == 0x800)
      {  
        tempOut_tmp = tempOut_tmp | 0xF000;
      }
      tempOut = (float)(tempOut_tmp) / 10;

      humidOut = 
        (code[31]=='1'?1:0)*8 +
        (code[30]=='1'?1:0)*4 +
        (code[29]=='1'?1:0)*2 +
        (code[28]=='1'?1:0)*1;
      humidOut = humidOut * 10 +
        (code[27]=='1'?1:0)*8 +
        (code[26]=='1'?1:0)*4 +
        (code[25]=='1'?1:0)*2 +
        (code[24]=='1'?1:0)*1;

      //battery state
      batt_combsensor=(code[8]=='1'?1:0)*1;
      //button
      tempOutButton=(code[11]=='1'?1:0);

      rec_time_TempHumid = millis();

      //      Serial.print(F(" tempOutButton="));
      //      Serial.println(tempOutButton);
      //      Serial.print(F(" tempOut="));
      //      Serial.println(tempOut);
      //      Serial.print(F(" humidOut="));
      //      Serial.println(humidOut);
    }
  }
}



//************************************************************
void getWindAverage(void)
{
  if 
    ((code[9]=='1')&&
    (code[10]=='1')&&
    (code[12]=='1')&&
    (code[13]=='0')&&
    (code[14]=='0')&&
    (code[15]=='0')&&
    (code[16]=='0')&&
    (code[17]=='0')&&
    (code[18]=='0')&&
    (code[19]=='0')&&
    (code[20]=='0')&&
    (code[21]=='0')&&
    (code[22]=='0')&&
    (code[23]=='0'))  
  {
    Serial.println(F("Received: WindAverage"));
    if(check_checksum(chksum_combsensor)==1)
    { 
      windAvg = 
        (code[31]=='1'?1:0)*128 +
        (code[30]=='1'?1:0)*64 +
        (code[29]=='1'?1:0)*32 +
        (code[28]=='1'?1:0)*16 +
        (code[27]=='1'?1:0)*8 +
        (code[26]=='1'?1:0)*4 +
        (code[25]=='1'?1:0)*2 +
        (code[24]=='1'?1:0)*1;
      windAvg = windAvg/5;

      //battery state
      batt_combsensor=(code[8]=='1'?1:0)*1;
      //button
      windButton=(code[11]=='1'?1:0);

      rec_time_WindAverage = millis();

      //      Serial.print(F(" windButton="));
      //      Serial.println(windButton);
      //      Serial.print(F(" windAvg="));
      //      Serial.println(windAvg);
    }
  }
}


//************************************************************
void getWindDirectionGust(void)
{
  if 
    ((code[9]=='1')&&
    (code[10]=='1')&&
    (code[12]=='1')&&
    (code[13]=='1')&&
    (code[14]=='1'))
  {     
    Serial.println(F("Received: WindDirectionGust"));
    if(check_checksum(chksum_combsensor)==1)
    {    
      windDir = 
        (code[23]=='1'?1:0)*256 +
        (code[22]=='1'?1:0)*128 +
        (code[21]=='1'?1:0)*64 +
        (code[20]=='1'?1:0)*32 +
        (code[19]=='1'?1:0)*16 +
        (code[18]=='1'?1:0)*8 +
        (code[17]=='1'?1:0)*4 +
        (code[16]=='1'?1:0)*2 +
        (code[15]=='1'?1:0)*1;
      windGust = 
        (code[31]=='1'?1:0)*128 +
        (code[30]=='1'?1:0)*64 +
        (code[29]=='1'?1:0)*32 +
        (code[28]=='1'?1:0)*16 +
        (code[27]=='1'?1:0)*8 +
        (code[26]=='1'?1:0)*4 +
        (code[25]=='1'?1:0)*2 +
        (code[24]=='1'?1:0)*1;

      windGust = windGust/5;

      //battery state
      batt_combsensor=(code[8]=='1'?1:0)*1;
      //button
      windButton=(code[11]=='1'?1:0);

      rec_time_WindDirectionGust = millis();

      //      Serial.print(F(" batt_combsensor="));
      //      Serial.println(batt_combsensor);
      //      Serial.print(F(" windButton="));
      //      Serial.println(windButton);
      //      Serial.print(F(" windDir="));
      //      Serial.println(windDir);
      //      Serial.print(F(" windGust="));
      //      Serial.println(windGust);
    }
  }
}


//************************************************************
void getRain(void)
{
  if 
    ((code[9]=='1')&&
    (code[10]=='1')&&
    (code[11]=='0')&&
    (code[12]=='1')&&
    (code[13]=='1')&&
    (code[14]=='0')&&
    (code[15]=='0'))
  {
    Serial.println(F("Received: Rain"));
    if(check_checksum(chksum_raingauge)==1)
    {
      rain = (code[31]=='1'?1:0)*32768 +
        (code[30]=='1'?1:0)*16384 +
        (code[29]=='1'?1:0)*8192 +
        (code[28]=='1'?1:0)*4096 +
        (code[27]=='1'?1:0)*2048 +
        (code[26]=='1'?1:0)*1024 +
        (code[25]=='1'?1:0)*512 +
        (code[24]=='1'?1:0)*256 +
        (code[23]=='1'?1:0)*128 +
        (code[22]=='1'?1:0)*64 +
        (code[21]=='1'?1:0)*32 +
        (code[20]=='1'?1:0)*16 +
        (code[19]=='1'?1:0)*8 +
        (code[18]=='1'?1:0)*4 +
        (code[17]=='1'?1:0)*2 +
        (code[16]=='1'?1:0)*1;
      rain = rain/4;

      //battery state
      batt_raingauge=(code[8]=='1'?1:0)*1;

      rec_time_Rain = millis();

      //substract initial rain value
      if(rain_initialshift == -1){
        rain_initialshift = rain;
      };
      rain = rain - rain_initialshift;


      //      Serial.print(F(" batt_raingauge="));
      //      Serial.println(batt_raingauge);
      //      Serial.print(F(" rain="));
      //      Serial.println(rain);
    }
  }  
}  



void SerialCode(void)
{
  //len vypis
  Serial.println();
  Serial.print(F("<"));
  for(int i = 0; i<9+0; i++){ //+1 for debug
    for(int ii = 0; ii<4; ii++){
      Serial.print(F(""));
      Serial.print(code[ii+i*4]);
    }
    Serial.print(F(" "));
  }    
  Serial.print(F(">"));
  Serial.println();  
}




void getVBat()
{
  float Vcc = readVcc();
  delay(2);
  float volt = analogRead(VBatPin);
  volt = (volt / 1023.0) * Vcc; // only correct if Vcc = 5.0 volts

  //R VCC 4.7 a 1k GND
  //VBat = volt * 5.73; //5.7
  VBat = volt / 175;
}





float getTempOut1(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_TempOut1.search(addr)) {
    //no more sensors on chain, reset search
    ds_TempOut1.reset_search();
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

  ds_TempOut1.reset();
  ds_TempOut1.select(addr);
  ds_TempOut1.write(0x44); // start conversion

  byte present = ds_TempOut1.reset();
  ds_TempOut1.select(addr);
  ds_TempOut1.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_TempOut1.read();
  }

  ds_TempOut1.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;


  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print(F("ERROR: CRC didn't match\n"));
    return -1000;
  }

  TempOut1 = TemperatureSum;

  return TemperatureSum;

}



float getTempOut2(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_TempOut2.search(addr)) {
    //no more sensors on chain, reset search
    ds_TempOut2.reset_search();
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

  ds_TempOut2.reset();
  ds_TempOut2.select(addr);
  ds_TempOut2.write(0x44); // start conversion

  byte present = ds_TempOut2.reset();
  ds_TempOut2.select(addr);
  ds_TempOut2.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_TempOut2.read();
  }

  ds_TempOut2.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;


  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print(F("ERROR: CRC didn't match\n"));
    return -1000;
  }

  TempOut2 = TemperatureSum;

  return TemperatureSum;

}













long readVcc() {
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



void CheckEth(void)
{
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if (pos){
    tone(buzzer,5000);
    
    bfill = ether.tcpOffset();

    Serial.print(F("Eth "));  
    bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));

    //data
    //if(recTimeoutStat == 0) 
    {
      addFloatTobfill(tempOut);
      addFloatTobfill(humidOut);
      addFloatTobfill(windDir);
      addFloatTobfill(windGust);
      addFloatTobfill(windAvg);
      addFloatTobfill(rain);
      addFloatTobfill(batt_combsensor);
      addFloatTobfill(batt_raingauge);

      addFloatTobfill(windDirAvg);

      addFloatTobfill(bmp085temperature);
      addFloatTobfill(bmp085pressure);

      addFloatTobfill(TempOut1);
      addFloatTobfill(TempOut2);
      addFloatTobfill(DHTTemp);
      addFloatTobfill(DHTHumid);

      addFloatTobfill(runtimeCounter);

//      addFloatTobfill(recTimeoutStat);  
//      addFloatTobfill(ethCounter);
    }

    if(recTimeoutStat > 0){
      bfill.emit_p(PSTR(" ERR: Sensor Timeout ->"));
      Serial.println(F(" ERR: Sensor Timeout ->"));
      int recTimeoutStat_tmp = recTimeoutStat;
      if(recTimeoutStat_tmp>7){ 
        bfill.emit_p(PSTR(" TempHumid")); 
        Serial.print(F(" TempHumid"));
        recTimeoutStat_tmp = recTimeoutStat_tmp -8; 
      }
      if(recTimeoutStat_tmp>3){ 
        bfill.emit_p(PSTR(" WindAverage")); 
        Serial.print(F(" WindAverage"));
        recTimeoutStat_tmp = recTimeoutStat_tmp -4; 
      }
      if(recTimeoutStat_tmp>1){ 
        bfill.emit_p(PSTR(" WindDirectionGust")); 
        Serial.print(F(" WindDirectionGust"));
        recTimeoutStat_tmp = recTimeoutStat_tmp -2; 
      }
      if(recTimeoutStat_tmp>0){ 
        bfill.emit_p(PSTR(" Rain")); 
        Serial.print(F(" Rain"));
        recTimeoutStat_tmp = recTimeoutStat_tmp -1; 
      }      
    } 




    ether.httpServerReply(bfill.position()) ;
    EtherQueryLast = time + EtherQueryTimeout;
    Serial.println();  
    ethCounter++;    
  }
  
  noTone(buzzer);
  
}


void getDHT(void){
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(t) || isnan(h)) {
    Serial.println(F("DHT: failed"));
  }
  else {
    DHTTemp = t;
    DHTHumid = h;
  }  

}


int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

