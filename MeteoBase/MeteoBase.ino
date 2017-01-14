#include <SPI.h>
#include <Mirf.h>
#include <MirfHardwareSpiDriver.h>
#include <Wire.h> //IIC
#include <math.h>
#include <OneWire.h>
#include <avr/wdt.h>

#define   nobits   40//36
#define   smin   7500
#define   smax   9900
#define   semin   250
#define   semax   750
#define   lmin   1700
#define   lmax   2300
#define   hmin   3700
#define   hmax   4300
#define ReceiverPin 2

const int VBatPin = A0;
const int TempOut2_DS18S20_Pin = 6;

//const unsigned long SendMirf_FirstTime = 300 *1000L; ////first data will be send 5 minutes after start, in ms
const unsigned long SendMirf_FirstTime = 1 *1000L; ////first data will be send 5 minutes after start, in ms
const unsigned long SendMirf_TimePeriod = 15 *1000L; //in ms

const unsigned long Watchdog_TimePeriod = 1 *1000L; //in ms
//const unsigned long Watchdog_TimePeriodReset = 86400 *1000L; //in sec


//timeout for receiving data from thermostat base
const unsigned long Rec_Timeout_Limit = 15 * 60000L;  //timeout for messages from Auriol
const unsigned long Rec_Timeout_StartValue = 99999999L; //to get timeout on start

unsigned long rec_time_TempHumid = Rec_Timeout_StartValue;
unsigned long rec_time_WindAverage = Rec_Timeout_StartValue;
unsigned long rec_time_WindDirectionGust = Rec_Timeout_StartValue;
unsigned long rec_time_Rain = Rec_Timeout_StartValue;

int recTimeoutStat;
const int Mirf_NUM_PACKETS = 5;
const int Mirf_payload = 16;
byte mirf_data[Mirf_payload];    
unsigned long SendMirf_UpdateTime = 0;
unsigned long Watchdog_UpdateTime = 0;
long testcounter;


String code, chksum_raingauge, chksum_combsensor;
int ID;
int batt_combsensor=-1, batt_raingauge=-1;  //initial value as not functional
int windButton;
int tempOutButton;
float tempOut;
int tempOut_tmp;
int humidOut;
float windGust;
int windDir;
float windAvg;
float rain;
float rain_initialshift = -1;

float VBat;
float TempOut2;


OneWire ds_TempOut2(TempOut2_DS18S20_Pin);

// ---------------------------------------------------------------------------------------
void setup() {

  wdt_enable(WDTO_8S);

  pinMode(VBatPin, INPUT);
  pinMode(ReceiverPin, INPUT);

  Wire.begin();

  Serial.begin(115200);

  Serial.println(); 
  Serial.print(F("Code "));
  Serial.print(F(__FILE__));   
  Serial.print(F(" built at: "));   
  Serial.print(F(__DATE__)); 
  Serial.print(F(" "));   
  Serial.println(F(__TIME__));
  Serial.println(F("setup();"));   

  delay(2);

  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setTADDR((byte *)"meteo");
  Mirf.payload = Mirf_payload;
  Mirf.config();
  delay(2);

  //first read to prepare sensors
  getTempOut2();
  //end of first read


  Serial.println(F("loop();"));

  //initialisation
  SendMirf_UpdateTime = SendMirf_FirstTime; 
  recTimeoutStat = 15;
  //end

}


void loop()
{
  //read weatherstation radio signal
  int ret_code = RCode();
  //decode weatherstation radio signal
  if(ret_code == 0){
    wdt_reset();
    //SerialCode();
    getGeneral();
    getRain();
    getWindDirectionGust();
    getWindAverage();
    getTempHumid();     
  }

  //send measured data to home
  if ((long)(millis() - SendMirf_UpdateTime > SendMirf_TimePeriod))   
  {
    wdt_reset();
    SendMirf_UpdateTime = millis();
    getVBat();
    getTempOut2();
    testcounter = millis() / 1000; 
    SendMirf();   
    getRecTimeoutStat();
    SerialAll();
  }


  //watchdog 
  if ((long)(millis() - Watchdog_UpdateTime > Watchdog_TimePeriod))   
    //if(millis() > Watchdog_UpdateTime)  
  {
    wdt_reset();
    Watchdog_UpdateTime = millis();
    //Serial.print(".");  

    //reset vzdy po nastavenom case
    /*    if(millis() > Watchdog_TimePeriodReset)  
     {
     Serial.println(F("Watchdog_TimePeriodReset!"));
     while(1);
     }
     */
  }
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

  //Serial.print(" recTimeoutStat=");
  //Serial.println(recTimeoutStat);  

  //recTimeoutStat = 0;
}



void SendMirf(){
  wdt_reset();
  Serial.println(F("SendMirf"));
  for(int i=1; i<=Mirf_NUM_PACKETS; i++){
    for(int ii=0; ii<20; ii++){ //repeat transfer x times
      SendMirf_packet(i);  //send each packet
      delay(2);
    }  
  }
}


void SendMirf_packet(int index)
{
  int i;
  byte mirf_data_checksum;

  Mirf.setTADDR((byte *)"meteo");
  delay(2);

  for (i = 0; i < Mirf.payload; i++) {
    mirf_data[i] = 0;
  }

  fillMirfPacket(index);

  //kontrolny sucet
  byte crc = 0x00;
  //mirf_data_checksum = 0; 
  for (i = 0; i < Mirf.payload-1; i++) {
    //mirf_data_checksum = mirf_data_checksum + mirf_data[i];
    byte extract = mirf_data[i];
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  mirf_data_checksum = crc;
  mirf_data[Mirf.payload-1] = mirf_data_checksum;


  Mirf.send((byte *) mirf_data);
  while(Mirf.isSending()){
    delay(1);
  }


  //  for (i = 0; i < 16; i++) {
  //    Serial.print(" ");
  //    Serial.print(mirf_data[i], DEC);
  //  }
  //  Serial.println();
}






void fillMirfPacket(int index){
  //filling array
  mirf_data[0] = index;

  if(index==1){
    mirf_data[1] = breakDownFloat(3, tempOut);     
    mirf_data[2] = breakDownFloat(2, tempOut);     
    mirf_data[3] = breakDownFloat(1, tempOut);     
    mirf_data[4] = breakDownFloat(0, tempOut);     
    mirf_data[5] = breakDownFloat(3, humidOut);     
    mirf_data[6] = breakDownFloat(2, humidOut);     
    mirf_data[7] = breakDownFloat(1, humidOut);     
    mirf_data[8] = breakDownFloat(0, humidOut);     
    mirf_data[9] = breakDownFloat(3, rain);  
    mirf_data[10] = breakDownFloat(2, rain);  
    mirf_data[11] = breakDownFloat(1, rain);  
    mirf_data[12] = breakDownFloat(0, rain);   
  }

  if(index==2){
    mirf_data[1] = breakDownFloat(3, windDir);  
    mirf_data[2] = breakDownFloat(2, windDir);  
    mirf_data[3] = breakDownFloat(1, windDir);  
    mirf_data[4] = breakDownFloat(0, windDir);
    mirf_data[5] = breakDownFloat(3, windGust);     
    mirf_data[6] = breakDownFloat(2, windGust);     
    mirf_data[7] = breakDownFloat(1, windGust);     
    mirf_data[8] = breakDownFloat(0, windGust);     
    mirf_data[9] = breakDownFloat(3, windAvg);     
    mirf_data[10] = breakDownFloat(2, windAvg);     
    mirf_data[11] = breakDownFloat(1, windAvg);     
    mirf_data[12] = breakDownFloat(0, windAvg);     

  }  

  if(index==3){
    mirf_data[1] = breakDownFloat(3, TempOut2);     
    mirf_data[2] = breakDownFloat(2, TempOut2);     
    mirf_data[3] = breakDownFloat(1, TempOut2);     
    mirf_data[4] = breakDownFloat(0, TempOut2);     
    mirf_data[5] = breakDownFloat(3, batt_combsensor);  
    mirf_data[6] = breakDownFloat(2, batt_combsensor);  
    mirf_data[7] = breakDownFloat(1, batt_combsensor);  
    mirf_data[8] = breakDownFloat(0, batt_combsensor);  
    mirf_data[9] = breakDownFloat(3, batt_raingauge);  
    mirf_data[10] = breakDownFloat(2, batt_raingauge);  
    mirf_data[11] = breakDownFloat(1, batt_raingauge);  
    mirf_data[12] = breakDownFloat(0, batt_raingauge);  
  }  

  if(index==4){
    mirf_data[1];
    mirf_data[2];
    mirf_data[3];
    mirf_data[4];
    mirf_data[5];
    mirf_data[6];
    mirf_data[7];
    mirf_data[8];
    mirf_data[9];
    mirf_data[10];
    mirf_data[11];
    mirf_data[12];
  }  

  if(index==5){
    mirf_data[1] = breakDownFloat(3, testcounter);     
    mirf_data[2] = breakDownFloat(2, testcounter);     
    mirf_data[3] = breakDownFloat(1, testcounter);     
    mirf_data[4] = breakDownFloat(0, testcounter);     
    mirf_data[5] = breakDownFloat(3, recTimeoutStat);     
    mirf_data[6] = breakDownFloat(2, recTimeoutStat);     
    mirf_data[7] = breakDownFloat(1, recTimeoutStat);     
    mirf_data[8] = breakDownFloat(0, recTimeoutStat);     
    mirf_data[9] = breakDownFloat(3, VBat);  
    mirf_data[10] = breakDownFloat(2, VBat);  
    mirf_data[11] = breakDownFloat(1, VBat);  
    mirf_data[12] = breakDownFloat(0, VBat);  
  }  

}









int breakDownFloat(int index, float input_f)
{
  input_f = input_f * 100;
  long input_l = input_f;
  unsigned char output_c;

  if(index==0){
    output_c = input_l & 0xFF;
  }

  if(index==1){
    output_c = (input_l & 0xFF00) >> 8;
  }

  if(index==2){
    output_c = (input_l & 0xFF0000) >> 16;
  }

  if(index==3){
    output_c = (input_l & 0xFF000000) >> 24;
  }

  return output_c;
}


float buildUpFloat(long outbox3, long  outbox2, long  outbox1, long outbox0)
{  
  long output_l =  (outbox3 << 24) | (outbox2 << 16) | (outbox1 << 8) | (outbox0); 
  float output_f = output_l;
  output_f = output_f / 100;
  return output_f;
}



















































