#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

#include <avr/wdt.h>  //watchdog

#include <VirtualWire.h> //433MHz

#define LED_ON  255
#define LED_OFF 0

const unsigned char rx433MHz_pin = 4;
const unsigned char tx433MHzUpdate_pin = 2;

uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
const unsigned char buflen_433MHz = 12;


//timeout for receiving data from thermostat base
const unsigned long rx433MHzPacket_Timeout = 10 * 60000;
#define rx433MHzPacket_MaxNum 4
unsigned long rx433MHzPacket[rx433MHzPacket_MaxNum];
const unsigned long rx433MHzAgain_StartValue = 9999999L; //to get timeout on start


//how often are data transmitted to thermostat base
//const unsigned long tx433MHzUpdate_Period = 1 * 60000;
//unsigned long tx433MHzUpdate;

//timeout for receiving data from pc server
const unsigned long rxEthPacket_Timeout = 10 * 60000;

unsigned long rxEthPacket[5];

int TempIzba1;
int TimeoutIzba1;
int TempBase;
int TempNast;
int Heating1, Heating2;
int TempVonku;
int SaunaTemp;
int RuryKotolVystup, RuryKotolSpiatocka, RuryBojlerVystup;
int TimeHH, TimeMM, TimeSS;

String params;

void setup()
{
  wdt_enable(WDTO_8S);

  //Serial.begin(115200);	// Debugging only
  //Serial.println(F("setup();"));

  uint8_t mac[6] = {
    0x74, 0x69, 0x69, 0x2D, 0x30, 0x35      };
  IPAddress myIP(192, 168, 1, 205);
  Ethernet.begin(mac, myIP);
  server.begin();

  vw_set_rx_pin(rx433MHz_pin);
  vw_set_tx_pin(tx433MHzUpdate_pin);

  vw_setup(2000);	 // Bits per sec
  vw_rx_start();       // Start the receiver PLL running

  randomSeed(analogRead(0));

  for(int i=0; i<rx433MHzPacket_MaxNum; i++){
    rx433MHzPacket[i] = rx433MHzAgain_StartValue;
  }

  //Serial.println(F("loop();"));
}

void loop()
{
  wdt_reset();

  check433MHz();

  checkEth();


/* removed as tx is performed immediately after eth receive
if (millis() - tx433MHzUpdate > tx433MHzUpdate_Period) {
    tx433MHzUpdate = millis() + random(1000);

    tx433MHz();
    //Serial.print(F("freeRam=")); Serial.println(freeRam());
  }
*/

}


/*
void tx433MHz(void)
{
  //Serial.println(F("tx433MHzUpdate();"));
  //Serial.print(F(" "));

  led(LED_ON);
  tx433MHzUpdate_packet(1);
  tx433MHzUpdate_packet(2);
  tx433MHzUpdate_packet(3);

  //tx433MHzUpdate_packet(4);
  //send all except packet 4 with TIME


  led(LED_OFF);
  //Serial.println();
}
*/


void tx433MHzUpdate_packet(byte packetNum)
{
  unsigned int tmptime;
  char msg[12] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
  };

  msg[0] = 'T';
  msg[1] = 'H';
  msg[2] = 'E';
  msg[3] = packetNum + 48;

  if (packetNum == 1) {
    msg[4] = TempVonku / 10;
    msg[5] = TempVonku % 10;
  }

  if (packetNum == 2) {
    msg[4] = SaunaTemp / 10;
    msg[5] = SaunaTemp % 10;
  }


  if (packetNum == 3) {
    msg[4] = RuryKotolVystup / 10;
    msg[5] = RuryKotolVystup % 10;
    msg[6] = RuryKotolSpiatocka / 10;
    msg[7] = RuryKotolSpiatocka % 10;
    msg[8] = RuryBojlerVystup / 10;
    msg[9] = RuryBojlerVystup % 10;
  }

  if (packetNum == 4) {
    msg[4] = TimeHH;
    msg[5] = TimeMM;
    msg[6] = TimeSS;
  }

  if (millis() - rxEthPacket[packetNum] < rxEthPacket_Timeout) {
    msg[10] = 0;
  }
  else {
    msg[10] = 1;
  }

  led(LED_ON);
  vw_send((uint8_t *)msg, buflen_433MHz);
  vw_wait_tx(); // Wait until the whole message is gone
  led(LED_OFF);

}




void check433MHz(void) {
  buflen = buflen_433MHz;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    //Message with a good checksum received, dump it.
    //Serial.println(F("VW_GET_MESSAGE"));
    int i;
    led(LED_ON);


    if ((buf[0] == 'T') && (buf[1] == 'H') && (buf[2] == 'B')) {

      if (buf[3] == '1') { //'1'
        rx433MHzPacket[1] = millis();
        TempIzba1 = convNumSigned(buf[4], buf[5]);
        TimeoutIzba1 = buf[10];
      }


      if (buf[3] == '2') { //'2'
        rx433MHzPacket[2] = millis();
        TempBase = convNumSigned(buf[4], buf[5]);
        TempNast = convNumSigned(buf[6], buf[7]);
        Heating1 = buf[8];
        Heating2 = buf[9];
      }
    }

    led(LED_OFF);
  }

}



int convNumSigned(int bufh, int bufl) {
  if (bufh > 127) {
    bufh = - ( 256 - bufh );
  }

  if (bufl > 127) {
    bufl = - ( 256 - bufl );
  }

  return (10 * (int)bufh + (int)bufl);
}





void checkEth() {
  size_t size;

  const String resVal = "/?";
  const char* medzera = " ";
  char c;

  if (EthernetClient client = server.available())
  {
    params = "";
    //Serial.println();
    //Serial.println(F("client.available()"));
    while ((size = client.available()) > 0)
    {
      char c = client.read();
      if (params.length() < 100) {
        if (c == '?') {
          params = "";
        }
        params.concat(String(c));
      }
    }
    params.concat('\0');

    if (params.charAt(0) == '?')
    {
      extractValuesFromparams();
      client.print(millis());
    }
    else
    {
      //Serial.println(F("answering"));
      client.print(TempIzba1);
      client.print(medzera);

      if ((long)(millis() -  rx433MHzPacket[1] < rx433MHzPacket_Timeout))
      {
        client.print("SENSOK");
      }
      else {
        client.print("SENSERR");
      }

      client.print(medzera);
      client.print(TempBase);
      client.print(medzera);
      client.print(TempNast);
      client.print(medzera);
      client.print(Heating1);
      client.print(medzera);
      client.print(Heating2);
      client.print(medzera);

      if ((long)(millis() -  rx433MHzPacket[2] < rx433MHzPacket_Timeout))
      {
        client.print("BASEOK");
      }
      else {
        client.print("BASEERR");
      }

      client.print(medzera);

      //      client.print(medzera);
      //      client.print("debug= ");
      //      client.print(TempVonku);
      //      client.print(medzera);
      //      client.print(SaunaTemp);
      //      client.print(medzera);
      //      client.print(RuryKotolVystup);
      //      client.print(medzera);
      //      client.print(RuryKotolSpiatocka);
      //      client.print(medzera);
      //      client.print(RuryBojlerVystup);
      //      client.print(medzera);

    }
    client.stop();
  }
}

int extractValuesFromparams(void)
{
  //Serial.println(F("extractValuesFromparams()"));
  //Serial.println(F(" params=")); Serial.println(params);

  for (int i = 0; i < 8; i++) {
    extractOneValuefromparams(i);
  }
}




void extractOneValuefromparams(unsigned char valueIndex)
{
  int ind1 = 0, ind2 = 0, ind3 = 0;
  int value = 0;
  String ethValuesNames[] = {
    "VoTe", "SaTe", "KoVy", "KoSp", "BoVy", "TiHH", "TiMM", "TiSS"
  };

  //Serial.println(F("extractOneValuefromparams()"));
  //Serial.print(F(" params=")); Serial.println(params);
  //Serial.print(F("freeRam=")); Serial.println(freeRam());

  ind1 = params.indexOf(ethValuesNames[valueIndex]);
  if (ind1 != -1) {
    //Serial.print(F("  ind1=")); Serial.print(ind1);
    ind2 = params.indexOf("=", ind1);
    if (ind2 != -1) {
      //Serial.print(F(" ind2=")); Serial.print(ind2);
      ind3 = params.indexOf("&", ind2);
      if (ind3 != -1) {
        //Serial.print(F(" ind3=")); Serial.println(ind3);

        //Serial.print(F(" params=")); Serial.println(params);
        String valueString = params.substring(ind2 + 1, ind3);
        //next line
        //value = (int)(10 * valueString.toFloat());
        //is replaced by
        char buffer[10];
        valueString.toCharArray(buffer, 10);
        value = (int)(10 * (float)(atof(buffer)));

        //Serial.print(F("  valuestring=")); Serial.println(valueString);

        //Serial.print(F(" valueName=")); Serial.println(ethValuesNames[valueIndex]);
        //Serial.print(F("  value=")); Serial.println(value);

        //Serial.print(F("freeRam=")); Serial.println(freeRam());

        if (valueIndex == 0) {
          TempVonku = value;
          rxEthPacket[1] = millis();
          
          //send immediately to thermostat_base immediately after receiving from pc
          tx433MHzUpdate_packet(1);
        }
        if (valueIndex == 1) {
          SaunaTemp = value;
          rxEthPacket[2] = millis();

          //send immediately to thermostat_base immediately after receiving from pc
          tx433MHzUpdate_packet(2);
        }

        if (valueIndex == 2) {
          RuryKotolVystup = value;
        }
        if (valueIndex == 3) {
          RuryKotolSpiatocka = value;
        }
        if (valueIndex == 4) {
          RuryBojlerVystup = value;
          rxEthPacket[3] = millis();

          //send immediately to thermostat_base immediately after receiving from pc
          tx433MHzUpdate_packet(3);
        }

        if (valueIndex == 5) {
          TimeHH = value / 10; //fraction part
        }
        if (valueIndex == 6) {
          TimeMM = value / 10; //fraction part
        }
        if (valueIndex == 7) {
          TimeSS = value / 10; //fraction part
          rxEthPacket[4] = millis();

          //send time to thermostat_base immediately after receiving from pc
          tx433MHzUpdate_packet(4);
        }

      }
    }
  }
}





void led(int value) {
  const unsigned char LED_PIN = 13;

  if (value > 0) {
    value = LED_ON;
  }
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, value);
}




int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}



















































