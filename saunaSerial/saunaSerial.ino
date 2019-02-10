#include <avr/wdt.h>

#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);


int i;

//sauna
int saunaskutTemp, saunanastTemp;
int saunacasVyp;
byte saunaohrevSpirala, saunatime;

const unsigned long EtherQuery_Timeout = 15 * 60000L;
unsigned long EtherQueryLast;

void setup()
{

  wdt_enable(WDTO_8S);

  wdt_reset();

  Serial.begin(115200);
  Serial.setTimeout(1000);

  uint8_t mac[6] = {
    0x74, 0x73, 0x73, 0x2D, 0x33, 0x37                                };
  IPAddress myIP(192, 168, 1, 203);
  Ethernet.begin(mac, myIP);
  server.begin();

}

void loop()
{
  wdt_reset();

  checkEth();

  checkSerial();

  //reset ak sa ethernet neozyva
  if ((long)(millis() - EtherQueryLast > EtherQuery_Timeout))
  {
#ifdef DEBUG
    Serial.println(F("EtherQueryLastTimeout"));
#endif
    while (1);
  }

}



void checkEth(void) {

  //ETHERNET PART
  if (EthernetClient client = server.available())
  {
    client.print(saunaskutTemp);
    client.print(F(" "));
    client.print(saunanastTemp);
    client.print(F(" "));
    client.print(saunacasVyp);
    client.print(F(" "));
    client.print(saunaohrevSpirala);
    client.print(F(" "));
    client.print(saunatime);
    client.print(F(" "));

    client.stop();
    EtherQueryLast = millis();
  }


}


void checkSerial(void)
{
  int in;

  if (Serial.available() > 30) {
    do{
      in = Serial.read();
    } while((in != 'S') && (Serial.available() > 0));

    if(in =='S'){
      saunaskutTemp = Serial.parseInt();
      saunanastTemp = Serial.parseInt();
      saunacasVyp = Serial.parseInt();
      saunaohrevSpirala = Serial.parseInt();
      saunatime = Serial.parseInt();
    }
  }
}



//void sendSerial(void)
//{
//  Serial.print ("A");
//  Serial.print (skutTemp);
//  Serial.print (" ");
//  Serial.print (nastTemp);
//  Serial.print (" ");
//  Serial.print (casVyp / 3600);
//  Serial.print (" ");
//  Serial.print (ohrevSpirala[1]);
//  Serial.print (" ");
//  Serial.print (ohrevSpirala[2]);
//  Serial.print (" ");
//  Serial.print (ohrevSpirala[3]);
//  Serial.print (" ");
//  Serial.print ("B");
//  Serial.println ();
//}



















































































