#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
//#include <EtherCard.h>   //stara kniznica, prejst na ether_28J60
#include <Wire.h>
#include <avr/wdt.h>

#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

const unsigned long DataReady_packet_TimePeriod = 10 *60 *1000L; //MIRF connection timeout 10 minutes in ms

const int Mirf_NUM_PACKETS = 5;
byte mirf_data[16];
byte mirf_data_checksum;
int i;

int recTimeoutStat;
int batt_combsensor=0, batt_raingauge=0;  //initial value as not functional
float tempOut=0;
int humidOut=0;
float windGust=0;
int windDir=0;
float windAvg=0;
float rain=0;


const unsigned long wdt_TimePeriod = 1 *1000L; //1 sec in ms
//const unsigned long wdt_TimePeriodReset = 86400 *1000L; //in sec
unsigned long wdt_TimeAgain = 0;


float TempOut2;
float UVindex;
uint16_t Light;
float SoilMoisture;
float SoilTemp;
float VBat;
long testcounter;

int windDirAvg= -1;
const int windDirAvg_Deg = 5;  //avg step in degrees
/*
// ethernet interface mac address
 static byte mymac[] = { 
 0x74,0x69,0x69,0x2D,0x30,0x32 };
 // ethernet interface ip address
 static byte myip[] = { 
 192,168,1,201 };
 // gateway ip address
 static byte gwip[] = { 
 192,168,1,1 };
 */
//byte Ethernet::buffer[500]; // tcp/ip send and receive buffer
//BufferFiller bfill;
char buff[12];

const long EtherQueryTimeout = 10 * 60 * 1000L;
unsigned long EtherQueryLast;

int DataReady_packet[10]; 
unsigned long DataReady_packet_TimeAgain[10];

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



void setup(){

  wdt_enable(WDTO_8S);

  Serial.begin(115200);

  //bmp085
  //Serial.println("bmp085 initialisation ");   
  Wire.begin();
  bmp085Calibration();

  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setRADDR((byte *)"meteo");
  Mirf.payload = 16;
  Mirf.config();

  uint8_t mac[6] = {
    0x74, 0x69, 0x69, 0x2D, 0x30, 0x32              };
  IPAddress myIP(192, 168, 1, 201);
  Ethernet.begin(mac, myIP);
  server.begin();

  EtherQueryLast =millis()+ EtherQueryTimeout;

  for(i=1; i<=Mirf_NUM_PACKETS; i++){
    DataReady_packet[i]=0;
    DataReady_packet_TimeAgain[i] =millis()+ DataReady_packet_TimePeriod;     
  }

  wdt_reset();

  Serial.println(); 
  Serial.println();
  Serial.println("MeteoEthernet is running ...");   


}

void loop(){

  checkMirf();

 // checkEth();

  checkWatchdog();

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



void SerialAll(void)
{
  Serial.print("  tempOut=");
  Serial.print(tempOut);
  Serial.print(", humidOut=");
  Serial.print(humidOut);
  Serial.print(", rain=");
  Serial.println(rain);

  Serial.print("  windDir=");
  Serial.print(windDir);
  Serial.print(", windGust="); 
  Serial.print(windGust);
  Serial.print(", windAvg=");
  Serial.println(windAvg);


  Serial.print("  TempOut2=");
  Serial.print(TempOut2);
  Serial.print(", UVindex=");
  Serial.print(UVindex);
  Serial.print(", Light=");
  Serial.println(Light);


  Serial.print("  SoilMoisture=");
  Serial.print(SoilMoisture);
  Serial.print(", SoilTemp=");
  Serial.println(SoilTemp);

  Serial.print("  batt_combsensor=");
  Serial.print(batt_combsensor);
  Serial.print(", batt_raingauge=");
  Serial.println(batt_raingauge);

  Serial.print("  recTimeoutStat=");
  Serial.print(recTimeoutStat);  
  Serial.print(", testcounter=");
  Serial.print(testcounter);
  Serial.print(", VBat=");
  Serial.println(VBat);  


  Serial.print(" bmp085temperature=");
  Serial.print(bmp085temperature);
  Serial.print(", bmp085pressure=");
  Serial.println(bmp085pressure);

  Serial.print(" EtherTimeout=");
  long tmp = (EtherQueryLast - millis()) / 1000;
  Serial.println(tmp);  

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





void checkMirf(void)
{
  //MIRF PART      
  if (Mirf.dataReady()){
    Mirf.getData((byte *) &mirf_data);

    Serial.print("Mirf received ");

        for (i = 0; i < Mirf.payload; i++) {
          Serial.print(" ");
          Serial.print(mirf_data[i], DEC);
        }    
        Serial.println();

    //kontrolny sucet predosla verzia
    //    mirf_data_checksum = 0; 
    //    for (i = 0; i < Mirf.payload-1; i++) {
    //      mirf_data_checksum = mirf_data_checksum + mirf_data[i];
    //    }

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



    if(mirf_data_checksum == mirf_data[Mirf.payload-1]){
      if(mirf_data[0] == 1)
      {
        tempOut = buildUpFloat(mirf_data[1], mirf_data[2], mirf_data[3], mirf_data[4]);
        humidOut = buildUpFloat(mirf_data[5], mirf_data[6], mirf_data[7], mirf_data[8]);
        rain = buildUpFloat(mirf_data[9], mirf_data[10], mirf_data[11], mirf_data[12]);
      }

      if(mirf_data[0] == 2)
      {
        windDir = buildUpFloat(mirf_data[1], mirf_data[2], mirf_data[3], mirf_data[4]);
        windGust = buildUpFloat(mirf_data[5], mirf_data[6], mirf_data[7], mirf_data[8]);
        windAvg = buildUpFloat(mirf_data[9], mirf_data[10], mirf_data[11], mirf_data[12]);
        Set_windDirAvg();
      }

      if(mirf_data[0] == 3)
      {
        TempOut2 = buildUpFloat(mirf_data[1], mirf_data[2], mirf_data[3], mirf_data[4]);
        UVindex = buildUpFloat(mirf_data[5], mirf_data[6], mirf_data[7], mirf_data[8]);
        Light = buildUpFloat(mirf_data[9], mirf_data[10], mirf_data[11], mirf_data[12]);
      }

      if(mirf_data[0] == 4)
      {
        SoilMoisture = buildUpFloat(mirf_data[1], mirf_data[2], mirf_data[3], mirf_data[4]);
        SoilTemp = buildUpFloat(mirf_data[5], mirf_data[6], mirf_data[7], mirf_data[8]);
        int batt_tmp = buildUpFloat(mirf_data[9], mirf_data[10], mirf_data[11], mirf_data[12]);
        batt_combsensor = batt_tmp / 2;
        batt_raingauge = batt_tmp - batt_combsensor * 2;
      }

      if(mirf_data[0] == 5)
      {
        testcounter = buildUpFloat(mirf_data[1], mirf_data[2], mirf_data[3], mirf_data[4]);
        recTimeoutStat = buildUpFloat(mirf_data[5], mirf_data[6], mirf_data[7], mirf_data[8]);
        VBat = buildUpFloat(mirf_data[9], mirf_data[10], mirf_data[11], mirf_data[12]);


      }

      if((mirf_data[0]>=1) && (mirf_data[0]<=Mirf_NUM_PACKETS)){
        DataReady_packet[mirf_data[0]] = 1;
        DataReady_packet_TimeAgain[mirf_data[0]] =millis()+ DataReady_packet_TimePeriod;     
        Serial.print("packet "); 
        Serial.print(mirf_data[0]); 
        Serial.print(" ");
      } 
      Serial.println("checksum ok"); 
    }
    else{
      Serial.println("checksum failed!");
    }

    //    for (i = 0; i < Mirf.payload; i++){ 
    //      mirf_data[i] = 0; //clear all 
    //    } 
    Mirf.getData((byte *) &mirf_data);
  }
} 



void checkWatchdog(void)
{

  //watchdog
  if(millis() > wdt_TimeAgain)  //watchdog
  {
    wdt_TimeAgain = millis()+ wdt_TimePeriod;
    wdt_reset();
    //Serial.print(".");  

    //reset vzdy po nastavenom case
    //if( (time > wdt_TimePeriodReset) || (EtherQueryLast < millis()) )
    if( EtherQueryLast < millis() )
    {
      while(1); //no Ethernet request for a long time, probably problem with communication, reset
    }

    for(i=1; i<=Mirf_NUM_PACKETS; i++){ 
      if(millis() > DataReady_packet_TimeAgain[i]){ 
        while(1); //reset arduino, probably problem with communication, reset
      }
    }        
  }

}




void checkEth(void)
{
  //ETHERNET PART        
  if (EthernetClient client = server.available())
  {
    Serial.println(F("Eth get"));

    //DataReady_packets check
    for(i=1; i<=Mirf_NUM_PACKETS; i++){ 
      if(millis() > DataReady_packet_TimeAgain[i]){ 
        DataReady_packet[i] == 0; 
      }  
    }

    client.print(" ");
    /*
    //data
     if((recTimeoutStat == 0) &&
     (DataReady_packet[1] == 1) && 
     (DataReady_packet[2] == 1) && 
     (DataReady_packet[3] == 1) && 
     (DataReady_packet[4] == 1) && 
     (DataReady_packet[5] == 1)){
     client.print(tempOut);
     client.print(F(" "));
     client.print(humidOut);
     client.print(F(" "));
     client.print(windDir);
     client.print(F(" "));
     client.print(windGust);
     client.print(F(" "));
     client.print(windAvg);
     client.print(F(" "));
     client.print(rain);
     client.print(F(" "));
     client.print(batt_combsensor);
     client.print(F(" "));
     client.print(batt_raingauge);
     client.print(F(" "));
     
     client.print(TempOut2);
     client.print(F(" "));
     client.print(UVindex);
     client.print(F(" "));
     client.print(Light);
     client.print(F(" "));
     client.print(SoilMoisture);
     client.print(F(" "));
     client.print(SoilTemp);
     client.print(F(" "));
     client.print(VBat);
     
     //bmp085
     bmp085temperature = (float)(bmp085GetTemperature(bmp085ReadUT()))/10;
     bmp085pressure = (float)bmp085GetPressure(bmp085ReadUP())/100 + BaseAltitudeCoef; //hpa
     if((bmp085pressure<900) || (bmp085pressure>1100)){ 
     while(1); //error check, if problem then restart
     }
     
     client.print(bmp085temperature);
     client.print(F(" "));
     client.print(bmp085pressure);
     client.print(F(" "));
     
     client.print(0);
     client.print(F(" "));
     client.print(windDirAvg);
     client.print(F(" "));
     
     client.print(recTimeoutStat);  
     client.print(F(" "));
     client.print(testcounter);
     client.print(F(" "));
     
     Serial.println();
     SerialAll(); 
     }
     
     if((recTimeoutStat > 0) &&
     (DataReady_packet[1] == 1) && 
     (DataReady_packet[2] == 1) && 
     (DataReady_packet[3] == 1) && 
     (DataReady_packet[4] == 1) && 
     (DataReady_packet[5] == 1)){
     client.print(F("ERR: Sensor Timeout ->"));
     int recTimeoutStat_tmp = recTimeoutStat;
     if(recTimeoutStat_tmp>7){ 
     client.print(F(" TempHumid")); 
     recTimeoutStat_tmp = recTimeoutStat_tmp -8; 
     }
     if(recTimeoutStat_tmp>3){ 
     client.print(F(" WindAverage")); 
     recTimeoutStat_tmp = recTimeoutStat_tmp -4; 
     }
     if(recTimeoutStat_tmp>1){ 
     client.print(F(" WindDirectionGust")); 
     recTimeoutStat_tmp = recTimeoutStat_tmp -2; 
     }
     if(recTimeoutStat_tmp>0){ 
     client.print(F(" Rain")); 
     recTimeoutStat_tmp = recTimeoutStat_tmp -1; 
     }
     Serial.println();
     Serial.println(" ERR: Sensor Timeout");
     }
     
     if ((DataReady_packet[1] != 1) || 
     (DataReady_packet[2] != 1) || 
     (DataReady_packet[3] != 1) || 
     (DataReady_packet[4] != 1) || 
     (DataReady_packet[5] != 1))
     {
     client.print(F("ERR: Transfer Timeout "));
     for(i=1; i<=Mirf_NUM_PACKETS; i++){
     client.print(i);
     if(DataReady_packet[i]==0){ 
     client.print(F("err "));
     };
     if(DataReady_packet[i]==1){ 
     client.print(F("ok "));
     };
     
     }
     Serial.println();
     Serial.println(" ERR: Transfer Timeout");
     }
     
     */

    client.stop();
    EtherQueryLast = millis()+ EtherQueryTimeout;
    Serial.println("sent");      

  }

}



