#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
SoftwareSerial RFID(2 , 3); // RX=2 and TX=3

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

int i, t, to, z;
int newRFID;
const int RFIDLengthMax = 14;
const long RFIDTimeoutMax = 300;
long RFIDTimeoutActValue;

enum newRFIDstate { 
  NoCard, CardPresent, NewCard, CardAlreadyRead };

enum RFIDmsgValid { 
  Empty, Valid };


int RFIDNewPos = 0;

struct sRFIDMsg {
  int b;
  int valid;
}; 

struct sRFIDMsg RFIDLast[RFIDLengthMax];
struct sRFIDMsg RFIDNew[RFIDLengthMax];


void setup()
{
  delay(1000);

  lcd.begin(16,2);               // initialize the lcd 
  //lcd.noCursor();
  lcd.backlight();

  RFID.begin(9600);    // start serial to RFID reader
  Serial.begin(115200);  // start serial to PC 


  lcd.clear();
  //lcd.print("zaciname....");

  //newRFID == NoCard;

}

void loop()
{
  //delay(1);

  if (RFID.available() > 0) 
  {
    RFIDTimeoutActValue = millis() + RFIDTimeoutMax;
    RFIDNew[RFIDNewPos].b = RFID.read();
    RFIDNew[RFIDNewPos].valid = Valid;
    //Serial.print(RFIDNew[RFIDNewPos]);
    RFIDNewPos++;    
  }


  if (RFID.available() == 0) {

    if (RFIDNewPos == RFIDLengthMax){
      for(i=0; i< RFIDLengthMax; i++){
        RFIDLast[i].b = RFIDNew[i].b;
        RFIDLast[i].valid = RFIDNew[i].valid;
        Serial.print(RFIDNew[i].b, HEX);
        Serial.print(" ");
        RFIDNew[i].valid = Empty;
      }
      RFIDNewPos = 0;
      Serial.print("\n");  
    }

    t = ((millis() / 500) % (RFIDLengthMax + 4)) - 4;    
    if(t != to){
      to = t;

      for(i = 0; i < 5; i++){        
        z = i+ t;
        lcd.setCursor(i*3,0);
        if((z >= 0) && (RFIDLast[z].valid == Valid)){
          lcd.print(RFIDLast[z].b, HEX);
        }    
        if(z == -1){
          lcd.print("id:");
        }
        lcd.print("   ");     
      }

    }



    if(RFIDTimeoutActValue < millis()){
      RFIDTimeoutActValue = millis(); 

      for(i=0; i< RFIDLengthMax; i++){
        RFIDLast[i].valid = Empty;
        //Serial.print("e ");  
      }


    }

  }

}





























