#include "etherShield.h"  //eth
#include "ETHER_28J60.h"  //eth

#include <avr/wdt.h>  //watchdog

#include <VirtualWire.h> //433MHz


// Define MAC address and IP address - both should be unique in your network
static uint8_t mac[6] = { 
  0x74,0x69,0x69,0x2D,0x30,0x35 }; //last number is like an IP
static uint8_t ip[4] = {
  192, 168, 1, 205};
static uint16_t port = 80; // Use port 80 - the standard for HTTP                                    

ETHER_28J60 ethernet;

const unsigned long rxEth_Timeout = 5 * 60 * 1000L;
unsigned long rxEthAgainPacket1, rxEthAgainPacket2, rxEthAgainPacket3;

const unsigned long tx433MHz_UpdateTimePeriod = 20 *1000L;
unsigned long tx433MHzUpdateTime;

const int rx433MHz_pin = 4; 
const int tx433MHz_pin = 2;
//byte txcount433MHz = 1;

uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;

const unsigned long rx433MHz_Timeout = 1 * 60 * 1000L;
unsigned long rx433MHzAgainPacket1, rx433MHzAgainPacket2, rx433MHzAgainPacket3;



unsigned long time;

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

  time = millis();

  check433MHz();

  checkEth();

  if(time > tx433MHzUpdateTime){
    tx433MHz();
    tx433MHzUpdateTime = time + tx433MHz_UpdateTimePeriod - random(10000);
  }

}



void tx433MHz(void)
{
  Serial.println(F("tx433MHz();"));

  if(rxEthAgainPacket1 > time){ 
    Serial.println(F("1"));
    tx433MHz_packet1(); 
  }

  if(rxEthAgainPacket2 > time){ 
    Serial.println(F("2"));
    tx433MHz_packet2(); 
  }

  if(rxEthAgainPacket3 > time){ 
    Serial.println(F("3"));
    tx433MHz_packet3(); 
  }
}



void tx433MHz_packet1(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                               };

  msg[0]='T';
  msg[1]='H';
  msg[2]='E';
  msg[3]='1';

/*
  if(tempVonku<0){ 
    msg[4]="-"; 
  }
  msg[5]= abs(TempVonku) / 10;
  msg[6]= abs(TempVonku) % 10;
  */
  
  //TempVonku = -123;
  //Serial.println(TempVonku);
  
  msg[4]= TempVonku / 10;
  msg[5]= TempVonku % 10;
  msg[6]= HumidityVonku / 10;
  msg[7]= HumidityVonku % 10;


  if(rxEthAgainPacket1 > time){ 
    msg[10]= 1; 
  } 
  else { 
    msg[10]= 0; 
  } 

  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
}



void tx433MHz_packet2(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                               };

  msg[0]='T';
  msg[1]='H';
  msg[2]='E';
  msg[3]='2';

  msg[4]= SaunaTemp / 10;
  msg[5]= SaunaTemp % 10;
  msg[6]= SaunaNast / 10;
  msg[7]= SaunaNast % 10;
  msg[8]= SaunaOhrev;

  if(rxEthAgainPacket2 > time){ 
    msg[10]= 1; 
  } 
  else { 
    msg[10]= 0; 
  } 

  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
}



void tx433MHz_packet3(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                               };

  msg[0]='T';
  msg[1]='H';
  msg[2]='E';
  msg[3]='3';

  msg[4]= RuryKotolVystup / 10;
  msg[5]= RuryKotolVystup % 10;
  msg[6]= RuryKotolSpiatocka / 10;
  msg[7]= RuryKotolSpiatocka % 10;
  msg[8]= RuryBojlerVystup / 10;
  msg[9]= RuryBojlerVystup % 10;


  if(rxEthAgainPacket3 > time){ 
    msg[10]= 1; 
  } 
  else { 
    msg[10]= 0; 
  } 

  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
}




void check433MHz(void){
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    //Message with a good checksum received, dump it.
    //Serial.println(F("VW_GET_MESSAGE"));
    //    int i;
    //    Serial.println(F("Received433MHz: "));	
    //    Serial.print(F(" ASC: "));	
    //    for (i = 0; i < buflen; i++)
    //    {
    //      char c;
    //      c = buf[i];
    //      Serial.print(c);
    //      Serial.print(" ");
    //    }
    //    Serial.println();
    //    //    Serial.print(F(" DEC: "));	
    //    for (i = 0; i < buflen; i++)
    //    {
    //      Serial.print(buf[i], DEC);
    //      Serial.print(" ");
    //    }
    //    Serial.println();


    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='B') &&
      (buf[3]=='1')){

      rx433MHzAgainPacket1 = time + rx433MHz_Timeout;

      TempIzba1 = convrxnum(buf[4], buf[5]); 
      HumidityIzba1 = convrxnum(buf[6], buf[7]); 
      TimeoutIzba1 = buf[10];
    }


    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='B') &&
      (buf[3]=='2')){

      rx433MHzAgainPacket2 = time + rx433MHz_Timeout;

      TempIzba2 = convrxnum(buf[4], buf[5]);  
      HumidityIzba2 = convrxnum(buf[6], buf[7]); 
      TimeoutIzba2 = buf[10];
    }


    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='B') &&
      (buf[3]=='3')){

      rx433MHzAgainPacket3 = time + rx433MHz_Timeout;

      TempBase = convrxnum(buf[4], buf[5]); 
      TempNast = convrxnum(buf[6], buf[7]); 
      Heating1 = buf[8];
      Heating2 = buf[9];
      Heating3 = buf[10];
    }


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
  if (paramEth = ethernet.serviceRequest())
  {
    Serial.println(F("checkEth();"));

    if(paramEth.length()==0){
      ethernet.print(TempIzba1); 
      ethernet.print(" "); 
      ethernet.print(HumidityIzba1);
      ethernet.print(" "); 
      ethernet.print(TimeoutIzba1); 
      ethernet.print(" "); 
      ethernet.print(TempIzba2); 
      ethernet.print(" "); 
      ethernet.print(HumidityIzba2);
      ethernet.print(" "); 
      ethernet.print(TimeoutIzba2); 
      ethernet.print(" "); 
      ethernet.print(TempBase);
      ethernet.print(" "); 
      ethernet.print(TempNast); 
      ethernet.print(" "); 
      ethernet.print(Heating1); 
      ethernet.print(" "); 
      ethernet.print(Heating2); 
      ethernet.print(" "); 
      ethernet.print(Heating3); 
      ethernet.print(" "); 
      ethernet.respond();
    }
    else {
      ethernet.respond();
      extractValuesFromparamEth(paramEth);

    }    
  }
}




int extractValuesFromparamEth(String paramEth)
{
  //int TempVonku=-123, HumidityVonku=340;
  //int SaunaTemp=742, SaunaNast=750, SaunaOhrev;
  //int RuryKotolVystup = 392, RuryKotolSpiatocka = 251, RuryBojlerVystup = 557;


  extractOneValuefromparamEth(paramEth, "VoTe"); 
  extractOneValuefromparamEth(paramEth, "VoHu"); 
  extractOneValuefromparamEth(paramEth, "SaTe"); 
  extractOneValuefromparamEth(paramEth, "SaNa"); 
  extractOneValuefromparamEth(paramEth, "SaOh"); 
  extractOneValuefromparamEth(paramEth, "KoVy"); 
  extractOneValuefromparamEth(paramEth, "KoSp"); 
  extractOneValuefromparamEth(paramEth, "BoVy"); 

}




int extractOneValuefromparamEth(String paramEth, String valueName)
{
  int ind1, ind2, ind3;
  int value;
  int retValue = 0;

  ind1 = paramEth.indexOf(valueName);
  if(ind1!=-1){ 
    ind2 = paramEth.indexOf("=", ind1); 
    if(ind2!=-1){ 
      ind3 = paramEth.indexOf("&", ind2); 
      String valueString = paramEth.substring(ind2+1, ind3);
      value = valueString.toInt();
      Serial.println(valueName);
      Serial.println(value);

      //int TempVonku=-123, HumidityVonku=340;
      //int SaunaTemp=742, SaunaNast=750, SaunaOhrev;
      //int RuryKotolVystup = 392, RuryKotolSpiatocka = 251, RuryBojlerVystup = 557;

      //value = value * 10;

      if(valueName == "VoTe"){ 
        TempVonku = value; 
        Serial.println(TempVonku);

      }
      if(valueName == "VoHu"){ 
        HumidityVonku = value; 
        rxEthAgainPacket1 = time + rxEth_Timeout;
      }
      if(valueName == "SaTe"){ 
        SaunaTemp = value; 
      }
      if(valueName == "SaNa"){ 
        SaunaNast = value; 
      }
      if(valueName == "SaOh"){ 
        SaunaOhrev = value; 
        rxEthAgainPacket2 = time + rxEth_Timeout;
      }
      if(valueName == "KoVy"){ 
        RuryKotolVystup = value; 
      }
      if(valueName == "KoSp"){ 
        RuryKotolSpiatocka = value; 
      }
      if(valueName == "BoVy"){ 
        RuryBojlerVystup = value; 
        rxEthAgainPacket3 = time + rxEth_Timeout;
      }

      return retValue;
    }
  }
}


































