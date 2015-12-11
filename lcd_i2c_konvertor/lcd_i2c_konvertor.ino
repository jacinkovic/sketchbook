//DFRobot.com
//Compatible with the Arduino IDE 1.0
//Library version:1.1
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


char incomingByte = 0;   // for incoming serial data
char incomingLine0[70];
char incomingLine1[70];
int incomingLineSize0;
int incomingLineSize1;

const int DisplayRows = 2;
const int DisplayCols = 16;
//LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x20 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27,DisplayCols,DisplayRows);  // set the LCD address to 0x20 for a 16 chars and 2 line display

int i;
int pos0=0;
int pos1=0;
char inData[70];
int inDataPos=0;


void setup()
{

  Serial.begin(115200);
  lcd.init();                      
  lcd.backlight();

  lcd.print("Go!");
  Serial.println("Go!");

  do{
    lcd.setCursor(0,3);
    lcd.print(millis());
  }
  while(1);
}

void loop()
{


  //while(1);



  while (Serial.available() > 0)
  {
    char received = Serial.read();
    inDataPos++;
    inData[inDataPos] = received; 
    Serial.print(inData[inDataPos]);
    Serial.print(inData[inDataPos],DEC);
    Serial.print(" ");
    Serial.println(inDataPos);

    // Process message when new line character is recieved
    if ((received == 10) || (inDataPos==63))
    {
      //move one line down
      incomingLineSize1=incomingLineSize0;
      pos1=0;
      for(i=0;i<incomingLineSize1;i++){
        incomingLine1[i]= incomingLine0[i];
      }


      //Serial.println();
      Serial.print("incomingLine: ");
      for(i=0; i<inDataPos; i++){
        incomingLine0[i] = inData[i+1];     
        Serial.print(incomingLine0[i]);   
        //lcd.print(incomingLine[0][i]); 
      }    

      incomingLineSize0 = i;
      inDataPos = 0; // Clear recieved buffer
      pos0=0;

      Serial.println(incomingLineSize0);
    }
  }






  if(pos0 > incomingLineSize0-DisplayCols-1){ 
    // if(pos0 > incomingLineSize0-DisplayCols/3){ 
    pos0=0;
  }
  if(pos1 > incomingLineSize1-DisplayCols-1){ 
    // if(pos1 > incomingLineSize1-DisplayCols/3){ 
    pos1=0;
  }

  //  Serial.println(pos0);
  //  Serial.println(incomingLineSize0);
  //  Serial.println(pos1);
  //  Serial.println(incomingLineSize1);

  if((pos0 == incomingLineSize0-DisplayCols-1) ||  
    (pos1 == incomingLineSize1-DisplayCols-1)){
  }

  lcd.setCursor(0,0);
  int pos0_end = pos0+DisplayCols;
  if(pos0_end>incomingLineSize0-1){ 
    pos0_end=incomingLineSize0-1;
  }

  for(i=pos0; i<pos0_end; i++){
    lcd.print(incomingLine0[i]); 
  }
  for(i=pos0_end-pos0; i<DisplayCols; i++){
    lcd.print(" ");
  }


  lcd.setCursor(0,1);
  int pos1_end = pos1+DisplayCols;
  if(pos1_end>incomingLineSize1-1){ 
    pos1_end=incomingLineSize1-1;
  }

  for(i=pos1; i<pos1_end; i++){
    lcd.print(incomingLine1[i]); 
  }
  for(i=pos1_end-pos1; i<DisplayCols; i++){
    lcd.print(" ");
  }


  pos0++;
  pos1++;

  delay(500);

  //first letter
  //  if( ((pos0==1) && (incomingLineSize0>DisplayCols)) || ((pos1==1) && (incomingLineSize1>DisplayCols)) ){    
  //    delay(350);
  //  }
  //if( (pos0==1) && (incomingLineSize0>DisplayCols)) || ((pos1==1) && (incomingLineSize1>DisplayCols)) ){    
  //delay(350);
  //}
}

































