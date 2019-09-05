#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/wdt.h>
#include <OneWire.h>
#include <UIPEthernet.h>
#include <EEPROM.h>
EthernetServer server = EthernetServer(80);

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

#define startEEPROMloc 100

void(* resetFunc) (void) = 0;//declare reset function at address 0

#define S 1uL
#define M 60uL
#define H 3600uL

#define DOPUMP    1
#define DONOTPUMP 0


#define PUMP1_PINOUTPUT        5
#define PUMP2_PINOUTPUT        6
#define PUMP3_PINOUTPUT        7

#define BTNPUMP1_PINOUTPUT     8
#define BTNPUMP2_PINOUTPUT     9
#define BTNPUMP3_PINOUTPUT    10
#define BTNSET_PINOUTPUT      11
#define BTNADJ_PINOUTPUT   12

const unsigned long WaitPeriod = 10000uL;
unsigned long WaitLast;

unsigned long ResetTime;

typedef struct pump_type
{
  unsigned long start;
  unsigned long duration;
  int EEPROMloc_start;
  int EEPROMloc_duration;
};

pump_type pump[4];

typedef struct lcdpos_type
{
  int x;
  int y;
  int pump;
  unsigned long timestep;
}; 

const lcdpos_type curpos_xy[] = { 
  1, 0, 1, H,    4, 0, 1, M,    2, 1, 1, S, 
  1+7, 0, 2, H,  4+7, 0, 2, M,  2+7, 1, 2, S,
  1+14, 0, 3, H, 4+14, 0, 3, M, 2+14, 1, 3, S, 
};   

const int curpos_loc_max = 9;
int curpos_loc = curpos_loc_max;


void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(9600);

  lcd.begin(20, 4);              // initialize the lcd
  lcd.clear();
  lcd.noCursor();
  lcd.backlight();
  lcd.home ();                   // go home


  pinMode(PUMP1_PINOUTPUT, OUTPUT);
  pinMode(PUMP2_PINOUTPUT, OUTPUT);
  pinMode(PUMP3_PINOUTPUT, OUTPUT);

  pinMode(BTNPUMP1_PINOUTPUT, INPUT);
  pinMode(BTNPUMP2_PINOUTPUT, INPUT);
  pinMode(BTNPUMP3_PINOUTPUT, INPUT);
  pinMode(BTNSET_PINOUTPUT, INPUT);
  pinMode(BTNADJ_PINOUTPUT, INPUT);

  dopump(1, DONOTPUMP);
  dopump(2, DONOTPUMP);
  dopump(3, DONOTPUMP);

  ResetTime = 24*H; //reset after end of cycle

  pump[1].EEPROMloc_start = startEEPROMloc;
  pump[1].EEPROMloc_duration = pump[1].EEPROMloc_start + 4;
  pump[2].EEPROMloc_start = pump[1].EEPROMloc_duration + 4;
  pump[2].EEPROMloc_duration = pump[2].EEPROMloc_start + 4;
  pump[3].EEPROMloc_start = pump[2].EEPROMloc_duration + 4;
  pump[3].EEPROMloc_duration = pump[3].EEPROMloc_start + 4;


  pump[1].start = EEPROMReadlong(pump[1].EEPROMloc_start); 
  pump[1].duration = EEPROMReadlong(pump[1].EEPROMloc_duration);  
  pump[2].start = EEPROMReadlong(pump[2].EEPROMloc_start); 
  pump[2].duration = EEPROMReadlong(pump[2].EEPROMloc_duration);  
  pump[3].start = EEPROMReadlong(pump[3].EEPROMloc_start); 
  pump[3].duration = EEPROMReadlong(pump[3].EEPROMloc_duration);  

if(pump[1].duration > M){ 
  pump[1].start = 0; pump[1].duration = 0; 
  pump[2].start = 0; pump[2].duration = 0; 
  pump[3].start = 0; pump[3].duration = 0; 
}
  

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



void printlcdtime(int hours, int minutes){ 
  if(hours<10){     
    lcd.print(F("0")); 
  }
  lcd.print(hours);
  lcd.print(F(":"));
  if(minutes<10){     
    lcd.print(F("0")); 
  }
  lcd.print(minutes);
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

int checkpump(unsigned long start, unsigned long duration)
{
  unsigned long currtime = millis()/1000;

  if ((currtime >= start) && (currtime < start+duration))
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

void dopump(int pumpnum, int state)
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


  if(state == DOPUMP){
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


  lcd.setCursor (0, 0);        
  //lcd.print(F("P1 "));
  cas(pump[1].start, &a.hours, &a.mins, &a.secs);
  printlcdtime(a.hours, a.mins);
  cas(pump[1].duration, &a.hours, &a.mins, &a.secs);
  lcd.setCursor (0+1, 1);        
  printlcdtime(a.secs);
  lcd.print(F("s"));


  lcd.setCursor (7, 0);        
  cas(pump[2].start, &a.hours, &a.mins, &a.secs);
  printlcdtime(a.hours, a.mins);
  cas(pump[2].duration, &a.hours, &a.mins, &a.secs);
  lcd.setCursor (7+1, 1);        
  printlcdtime(a.secs);
  lcd.print(F("s"));

  lcd.setCursor (14, 0);        
  cas(pump[3].start, &a.hours, &a.mins, &a.secs);
  printlcdtime(a.hours, a.mins);
  cas(pump[3].duration, &a.hours, &a.mins, &a.secs);
  lcd.setCursor (14+1, 1);        
  printlcdtime(a.secs);
  lcd.print(F("s"));


  //lcd.setCursor (0, 3);        
  //    lcd.print(F("192.168.1.207"));
  cas(millis()/1000,&a.hours, &a.mins, &a.secs);

  lcd.setCursor (0, 3);        
  //lcd.print(F("TIME:"));
  printlcdtime(a.hours, a.mins, a.secs);


  lcd.setCursor (0, 2);   
  if((checkpump(pump[1].start, pump[1].duration)==1) || (digitalRead(BTNPUMP1_PINOUTPUT) == HIGH))
  {
    lcd.print("^^^^^");
    dopump(1, DOPUMP);
  }
  else
  {
    lcd.print(F("     "));
    dopump(1, DONOTPUMP);
  }


  lcd.setCursor (7, 2);   
  if((checkpump(pump[2].start, pump[2].duration)==1) || (digitalRead(BTNPUMP2_PINOUTPUT) == HIGH))
  {
    lcd.print("^^^^^");
    dopump(2, DOPUMP);
  }
  else
  {     
    lcd.print(F("     "));
    dopump(2, DONOTPUMP);
  }


  lcd.setCursor (14, 2);   
  if((checkpump(pump[3].start, pump[3].duration)==1) || (digitalRead(BTNPUMP3_PINOUTPUT) == HIGH))
  {
        lcd.print("^^^^^");
        dopump(3, DOPUMP);
  }
  else
  {     
    lcd.print(F("     "));
    dopump(3, DONOTPUMP);
  }

  //choosing which value to change
  if( (digitalRead(BTNSET_PINOUTPUT) == HIGH)){
    WaitLast = millis();
    curpos_loc++;
    if(curpos_loc > curpos_loc_max){ 
      curpos_loc = 0; 
    }
  }

  if(curpos_loc == curpos_loc_max){
    lcd.noCursor();
  }
  else {  
    lcd.cursor();
  }

  //changing particular value if in set mode
  if(curpos_loc < curpos_loc_max){
    if( (digitalRead(BTNADJ_PINOUTPUT) == HIGH)){  
      WaitLast = millis();

      //hours and minutes only for starttime
      if(curpos_xy[curpos_loc].timestep == H){
        cas(pump[curpos_xy[curpos_loc].pump].start, &a.hours, &a.mins, &a.secs);
        if(a.hours==23){ 
          pump[curpos_xy[curpos_loc].pump].start -= 23*H;
        } 
        else {
          pump[curpos_xy[curpos_loc].pump].start += H;
        }
      }

      //hours and minutes only for starttime
      if(curpos_xy[curpos_loc].timestep == M){
        cas(pump[curpos_xy[curpos_loc].pump].start, &a.hours, &a.mins, &a.secs);
        if(a.mins==59){ 
          pump[curpos_xy[curpos_loc].pump].start -= 59*M;
        } 
        else {
          pump[curpos_xy[curpos_loc].pump].start += M;
        }
      }

      //seconds only for duration
      if(curpos_xy[curpos_loc].timestep == S){
        cas(pump[curpos_xy[curpos_loc].pump].duration, &a.hours, &a.mins, &a.secs);
        if(a.secs==59){ 
          pump[curpos_xy[curpos_loc].pump].duration -= 59*S;
        } 
        else {
          pump[curpos_xy[curpos_loc].pump].duration += S;
        }
      }

      EEPROMWritelong(pump[1].EEPROMloc_start, pump[1].start); 
      EEPROMWritelong(pump[1].EEPROMloc_duration, pump[1].duration); 
      EEPROMWritelong(pump[2].EEPROMloc_start, pump[2].start); 
      EEPROMWritelong(pump[2].EEPROMloc_duration, pump[2].duration); 
      EEPROMWritelong(pump[3].EEPROMloc_start, pump[3].start); 
      EEPROMWritelong(pump[3].EEPROMloc_duration, pump[3].duration); 

    }
  }



  lcd.setCursor(curpos_xy[curpos_loc].x,curpos_xy[curpos_loc].y);

  if((millis()) > (WaitLast + WaitPeriod))
  {
    //no iput from user, cancel setting mode
    curpos_loc = curpos_loc_max;  
  }


  if ((millis()/1000) >= ResetTime)
  {
    resetFunc(); //call reset  
  }

  delay(100);


  //lcd.setCursor (18, 3);   
  //lcd.print(curpos_loc); 
  //lcd.print(" ");
}




long EEPROMReadlong(int address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //avoid overwriting with the same values
  if(EEPROMReadlong(address) != value)
  {
    EEPROM.write(address, four);
    EEPROM.write(address + 1, three);
    EEPROM.write(address + 2, two);
    EEPROM.write(address + 3, one);
  }
}







































