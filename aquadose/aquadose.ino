#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <UIPEthernet.h>
EthernetServer server = EthernetServer(80);

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

unsigned long P1starttime, P1pumptime;
unsigned long P2starttime, P2pumptime;
unsigned long P3starttime, P3pumptime;
unsigned long ResetTime;

const unsigned long WaitPeriod = 1000L;
unsigned long WaitLast;

void(* resetFunc) (void) = 0;//declare reset function at address 0

#define S 1uL
#define M 60uL
#define H 3600uL


#define PUMP     1
#define NOPUMP   0


#define PUMP1_PINOUTPUT  5
#define PUMP2_PINOUTPUT  6
#define PUMP3_PINOUTPUT  7



void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(9600);

//  P1starttime = 12*H; //in minutes
//  P1pumptime = 7*S;  //in secs
//  P2starttime = 23*H+59*M; 
//  P2pumptime = 15*S;
//  P3starttime = 0*H+0*M+0*S; //reset after do
//  P3pumptime = 0*S;
//  ResetTime = 24*H; //reset after end of cycle

  P1starttime = 10*S; //in minutes
  P1pumptime = 7*S;  //in secs
  P2starttime = 20*S; 
  P2pumptime = 5*S;
  P3starttime = 35*S; //reset after do
  P3pumptime = 7*S;
  ResetTime = 2*M; //reset after end of cycle


  lcd.begin(20, 4);              // initialize the lcd
  lcd.clear();
  lcd.noCursor();
  lcd.backlight();
  lcd.home ();                   // go home
  lcd.print(F("AQUADOSE 2019"));

  pinMode(PUMP1_PINOUTPUT, OUTPUT);
  pinMode(PUMP2_PINOUTPUT, OUTPUT);
  pinMode(PUMP3_PINOUTPUT, OUTPUT);

  pump(1, NOPUMP);
  pump(2, NOPUMP);
  pump(3, NOPUMP);

  //  uint8_t mac[6] = {
  //    0x74, 0x73, 0x71, 0x2D, 0x32, 0x37                         };
  //  IPAddress myIP(192, 168, 1, 207);
  //  Ethernet.begin(mac, myIP);
  //server.begin();

}

long *minutes;
long *seconds;

int cas (unsigned long time, int *hours, int *minutes, int *seconds)
{
  long h, m, s;

  h = time / H;
  m = (time - h*H) / M;
  s = (time - h*H - m*M) / S;

  *hours = h;
  *minutes = m;
  *seconds = s;

  return 0;
}



void printlcdtime(int hours, int minutes, int seconds){ 
  if(hours<10){     
    lcd.print(F("0")); 
  }
  lcd.print(hours);
  lcd.print(F(":"));
  if(minutes<10){     
    lcd.print(F("0")); 
  }
  lcd.print(minutes);
  lcd.print(F(":"));
  if(seconds<10){     
    lcd.print(F("0")); 
  }
  lcd.print(seconds);

}  

void printlcdtime(int seconds){ 
  if(seconds<10){     
    lcd.print(F("0")); 
  }
  lcd.print(seconds);
}  

struct time{
  int hours;
  int mins;
  int secs;
} 
a;

int checkpump(unsigned long starttime, unsigned long pumptime)
{
  unsigned long currtime = millis()/1000;

  if ((currtime >= starttime) && (currtime < starttime+pumptime))
  {
    //time to pump
    return 1;
  }
  else
  {
    //do nothing
    return 0;
  }

};

void pump(int pumpnum, int state)
{
  int pin;

  switch(pumpnum)
  {
  case 1:
    pin = PUMP1_PINOUTPUT;
    break;
  case 2:
    pin = PUMP2_PINOUTPUT;
    break;
  case 3:
    pin = PUMP3_PINOUTPUT;
    break;
  default:
    break;
  }


  if(state == PUMP){
    digitalWrite(pin, LOW);
  } 
  else {
    digitalWrite(pin, HIGH);
  }

}



void loop()
{


  wdt_reset();

  //ETHERNET PART
  //  if (EthernetClient client = server.available())
  //  {
  //    Serial.print(F("eth. "));
  //    client.print(co2);
  //    client.print(F(" "));
  //    client.print(humidout);
  //    client.stop();
  //    EtherQueryLast = millis();
  //  }


  lcd.setCursor (1, 1);        
  lcd.print(F("P1 "));
  cas(P1starttime, &a.hours, &a.mins, &a.secs);
  printlcdtime(a.hours, a.mins, a.secs);
  cas(P1pumptime, &a.hours, &a.mins, &a.secs);
  lcd.print(F(" "));
  printlcdtime(a.secs);
  lcd.print(F("s "));


  lcd.setCursor (1, 2);        
  lcd.print(F("P2 "));
  cas(P2starttime, &a.hours, &a.mins, &a.secs);
  printlcdtime(a.hours, a.mins, a.secs);
  cas(P2pumptime, &a.hours, &a.mins, &a.secs);
  lcd.print(F(" "));
  printlcdtime(a.secs);
  lcd.print(F("s "));

  lcd.setCursor (1, 3);        
  lcd.print(F("P3 "));
  cas(P3starttime, &a.hours, &a.mins, &a.secs);
  printlcdtime(a.hours, a.mins, a.secs);
  cas(P3pumptime, &a.hours, &a.mins, &a.secs);
  lcd.print(F(" "));
  printlcdtime(a.secs);
  lcd.print(F("s "));


  //lcd.setCursor (0, 3);        
  //    lcd.print(F("192.168.1.207"));
  cas(millis()/1000,&a.hours, &a.mins, &a.secs);

  lcd.setCursor (0, 0);        
  lcd.print(F("ACT "));
  printlcdtime(a.hours, a.mins, a.secs);
  lcd.print(F(" "));


  lcd.setCursor (0, 1);   
  if(checkpump(P1starttime, P1pumptime)==1)
  {
    lcd.write(B11011011); //pump   
    pump(1, PUMP);
  }
  else
  {
    lcd.write(B00101101); //nopump
    pump(1, NOPUMP);
  }


  lcd.setCursor (0, 2);   
  if(checkpump(P2starttime, P2pumptime)==1)
  {
    lcd.write(B11011011); //pump   
    pump(2, PUMP);
  }
  else
  {     
    lcd.write(B00101101); //nopump
    pump(2, NOPUMP);
  }


  lcd.setCursor (0, 3);   
  if(checkpump(P3starttime, P3pumptime)==1)
  {
    lcd.write(B11011011); //pump
    pump(3, PUMP);
  }
  else
  {     
    lcd.write(B00101101); //nopump
    pump(3, NOPUMP);
  }


  if ((millis()/1000) >= ResetTime)
  {
    resetFunc(); //call reset  
  }

  delay(100);
}




























