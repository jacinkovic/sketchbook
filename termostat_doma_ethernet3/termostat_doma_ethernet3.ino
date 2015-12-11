#include "etherShield.h"  //eth
#include "ETHER_28J60.h"  //eth

#include <avr/wdt.h>  //watchdog

#include <VirtualWire.h> //433MHz

const unsigned char LED_on = 255;
const unsigned char LED_off = 0;

// Define MAC address and IP address - both should be unique in your network
static uint8_t mac[6] = { 
  0x74,0x69,0x69,0x2D,0x30,0x35 }; //last number is like an IP
static uint8_t ip[4] = {
  192, 168, 1, 205};
static uint16_t port = 80; // Use port 80 - the standard for HTTP                                    

ETHER_28J60 ethernet;

const unsigned int rxEth_Timeout = 15 * 60;
unsigned int rxEthAgainPacket[4];

const unsigned int tx433MHz_UpdateTimePeriod = 1 *60;
unsigned int tx433MHzUpdateTime;

const unsigned char rx433MHz_pin = 4; 
const unsigned char tx433MHz_pin = 2;

uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
const unsigned char buflen_433MHz = 12;

const unsigned int rx433MHz_Timeout = 1 * 60;
unsigned int rx433MHzAgainPacket[4];

unsigned int time;

int TempIzba1, HumidityIzba1;
int TimeoutIzba1;
int TempIzba2, HumidityIzba2;
int TimeoutIzba2;
int TempBase;
int TempNast;
int Heating1, Heating2, Heating3;
//int TempVonku=-123, HumidityVonku=340;
//int SaunaTemp=742, SaunaNast=750, SaunaOhrev;
//int RuryKotolVystup = 392, RuryKotolSpiatocka = 251, RuryBojlerVystup = 557;
int TempVonku, HumidityVonku;
int SaunaTemp, SaunaNast, SaunaOhrev;
int RuryKotolVystup, RuryKotolSpiatocka, RuryBojlerVystup;

//String ethValuesNames[8] = {
//  "VoTe", "VoHu", "SaTe", "SaNa", "SaOh", "KoVy", "KoSp", "BoVy"}; 

void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(115200);	// Debugging only
  Serial.println(F("setup();"));

  ethernet.setup(mac, ip, port);

  vw_set_rx_pin(rx433MHz_pin);
  vw_set_tx_pin(tx433MHz_pin);

  vw_setup(2000);	 // Bits per sec
  vw_rx_start();       // Start the receiver PLL running

  randomSeed(analogRead(0));

  Serial.println(F("loop();"));
}

void loop()
{
  wdt_reset();

  time = (unsigned long)(millis() / 1000);

  check433MHz();

  checkEth();

  if(time > tx433MHzUpdateTime){
    tx433MHz();
    Serial.println(freeRam());
    tx433MHzUpdateTime = time + tx433MHz_UpdateTimePeriod - random(10);
  }


}



void tx433MHz(void)
{
  Serial.println(F("tx433MHz();"));
  Serial.print(F(" "));

  led(LED_on);
  if(rxEthAgainPacket[1] > time){ 
    Serial.print(F("1"));
    tx433MHz_packet(1); 
  }

  if(rxEthAgainPacket[2] > time){ 
    Serial.print(F("2"));
    tx433MHz_packet(2); 
  }

  if(rxEthAgainPacket[3] > time){ 
    Serial.print(F("3"));
    tx433MHz_packet(3); 
  }

  led(LED_off);
  Serial.println();
}



void tx433MHz_packet(byte packetNum)
{
  unsigned int tmptime;
  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                                                                                 };

  msg[0]='T';
  msg[1]='H';
  msg[2]='E';
  msg[3]=packetNum + 48;

  if(packetNum == 1){
    msg[4]= TempVonku / 10;
    msg[5]= TempVonku % 10;
    msg[6]= HumidityVonku / 10;
    msg[7]= HumidityVonku % 10;
  }

  if(packetNum == 2){
    msg[4]= SaunaTemp / 10;
    msg[5]= SaunaTemp % 10;
    msg[6]= SaunaNast / 10;
    msg[7]= SaunaNast % 10;
    msg[8]= SaunaOhrev;
  }


  if(packetNum == 3){
    msg[4]= RuryKotolVystup / 10;
    msg[5]= RuryKotolVystup % 10;
    msg[6]= RuryKotolSpiatocka / 10;
    msg[7]= RuryKotolSpiatocka % 10;
    msg[8]= RuryBojlerVystup / 10;
    msg[9]= RuryBojlerVystup % 10;
  }

  if(rxEthAgainPacket[packetNum] > time){ 
    msg[10]= 1; 
  } 
  else { 
    msg[10]= 0; 
  } 

  vw_send((uint8_t *)msg, buflen_433MHz);
  vw_wait_tx(); // Wait until the whole message is gone
}




void check433MHz(void){
  buflen = buflen_433MHz;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    //Message with a good checksum received, dump it.
    //Serial.println(F("VW_GET_MESSAGE"));
    int i;
    led(LED_on);
    Serial.println(F("Received433MHz: "));	
    Serial.print(F(" buflen="));        
    Serial.println(buflen);
    Serial.print(F(" ASC: "));	
    for (i = 0; i < buflen; i++)
    {
      char c;
      c = buf[i];
      Serial.print(c);
      Serial.print(" ");
    }
    Serial.println();
    //    //    Serial.print(F(" DEC: "));	
    //    for (i = 0; i < buflen; i++)
    //    {
    //      Serial.print(buf[i], DEC);
    //      Serial.print(" ");
    //    }
    //    Serial.println();


    if((buf[0]=='T') && (buf[1]=='H') && (buf[2]=='B')){

      if(buf[3]=='1'){  //'1'
        rx433MHzAgainPacket[1] = time + rx433MHz_Timeout;
        TempIzba1 = convrxnum(buf[4], buf[5]); 
        HumidityIzba1 = convrxnum(buf[6], buf[7]); 
        TimeoutIzba1 = buf[10];
      }


      if(buf[3]=='2'){  //'2'
        rx433MHzAgainPacket[2] = time + rx433MHz_Timeout;
        TempIzba2 = convrxnum(buf[4], buf[5]);  
        HumidityIzba2 = convrxnum(buf[6], buf[7]); 
        TimeoutIzba2 = buf[10];
      }


      if(buf[3]=='3'){  //'3'
        rx433MHzAgainPacket[3] = time + rx433MHz_Timeout;
        TempBase = convrxnum(buf[4], buf[5]); 
        TempNast = convrxnum(buf[6], buf[7]); 
        Heating1 = buf[8];
        Heating2 = buf[9];
        Heating3 = buf[10];
      }
    }

    led(LED_off);
  }

}



int convrxnum(uint8_t bufh, uint8_t bufl){
  if(bufh > 127){ 
    return -( 10 * (256 - (int)bufh ) + ( 256 - (int)bufl) );
  } 
  else {
    return (10* (int)bufh + (int)bufl);  
  }
}



void checkEth(){
  String paramEth;

  char* medzera = " ";

  if (paramEth = ethernet.serviceRequest())
  {
    Serial.println(F("checkEth();"));

    if(paramEth.length()==0){
      ethernet.print(TempIzba1); 
      ethernet.print(medzera); 
      ethernet.print(HumidityIzba1);
      ethernet.print(medzera);        
      ethernet.print(TimeoutIzba1); 
      ethernet.print(medzera); 
      ethernet.print(TempIzba2); 
      ethernet.print(medzera); 
      ethernet.print(HumidityIzba2);
      ethernet.print(medzera); 
      ethernet.print(TimeoutIzba2); 
      ethernet.print(medzera); 
      ethernet.print(TempBase);
      ethernet.print(medzera); 
      ethernet.print(TempNast); 
      ethernet.print(medzera); 
      ethernet.print(Heating1); 
      ethernet.print(medzera); 
      ethernet.print(Heating2); 
      ethernet.print(medzera); 
      ethernet.print(Heating3); 
      ethernet.print(medzera); 
      ethernet.print((millis()/1000)); 
      ethernet.print(medzera); 
      ethernet.print(buflen); 
      ethernet.print(medzera); 
      ethernet.respond();
    }
    else {
      ethernet.respond();
      extractValuesFromparamEth(paramEth);

      //      Serial.println(TempVonku);
      //      Serial.println(HumidityVonku);
      //      Serial.println(SaunaTemp);
      //      Serial.println(SaunaNast);
      //      Serial.println(SaunaOhrev);
      //      Serial.println(RuryKotolVystup);
      //      Serial.println(RuryKotolSpiatocka);
      //      Serial.println(RuryBojlerVystup);

    }    
  }
}




int extractValuesFromparamEth(String paramEth)
{
  String ethValuesNames[8] = {
    "VoTe", "VoHu", "SaTe", "SaNa", "SaOh", "KoVy", "KoSp", "BoVy"            }; 

  for(int i=0; i<8; i++){
    extractOneValuefromparamEth(paramEth, ethValuesNames[i]); 
  }
}




int extractOneValuefromparamEth(String paramEth, String valueName)
{
  String ethValuesNames[8] = {
    "VoTe", "VoHu", "SaTe", "SaNa", "SaOh", "KoVy", "KoSp", "BoVy"            }; 

  int ind1, ind2, ind3;
  int value;
  int retValue = 0;

  ind1 = paramEth.indexOf(valueName);
  if(ind1!=-1){ 
    ind2 = paramEth.indexOf("=", ind1); 
    if(ind2!=-1){ 
      ind3 = paramEth.indexOf("&", ind2); 
      String valueString = paramEth.substring(ind2+1, ind3);
      value = (int)(10 * valueString.toFloat());

      Serial.print(F(" "));
      Serial.print(valueName);
      Serial.print(F(" "));
      Serial.println(value);


      if(valueName == ethValuesNames[0]){ 
        TempVonku = value; 
      }
      if(valueName == ethValuesNames[1]){ 
        HumidityVonku = value; 
        rxEthAgainPacket[1] = time + rxEth_Timeout;
      }
      if(valueName == ethValuesNames[2]){ 
        SaunaTemp = value; 
      }
      if(valueName == ethValuesNames[3]){ 
        SaunaNast = value; 
      }
      if(valueName == ethValuesNames[4]){ 
        SaunaOhrev = value; 
        rxEthAgainPacket[2] = time + rxEth_Timeout;
      }
      if(valueName == ethValuesNames[5]){ 
        RuryKotolVystup = value; 
      }
      if(valueName == ethValuesNames[6]){ 
        RuryKotolSpiatocka = value; 
      }
      if(valueName == ethValuesNames[7]){ 
        RuryBojlerVystup = value; 
        rxEthAgainPacket[3] = time + rxEth_Timeout;
      }

      return retValue;
    }
  }
}






void led(int value){
  const unsigned char LED_pin=13;

  if(value>0){
    value=LED_on;
  }
  pinMode(LED_pin, OUTPUT);
  analogWrite(LED_pin, value);
}




int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
















































