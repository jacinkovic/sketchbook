#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#include <SoftwareSerial.h>

#define DEBUG true

SoftwareSerial esp8266(2,3); // make RX Arduino line is pin 2, make TX Arduino line is pin 3.
// This means that you need to connect the TX line from the esp to the Arduino's pin 2
// and the RX line from the esp to the Arduino's pin 3

const int c_Length = 15;
int i;
char c[c_Length + 2];
String c_string;

char wifiNamePass[2][12];
String wifiName, wifiPass;


void setup()
{
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.setCursor(0,0);

  Serial.begin(115200);

  wifiNamePassConversion();
  esp8266.begin(9600); // your esp's baud rate might be different

  Serial.println("--- Start! ---");

  //while((sendData("AT+RST\r\n",10000))!=0); // reset module
  //while((sendData("AT+CWMODE=1\r\n",3000))!=0); //AP client mode
  //while((sendData("AT+CWJAP=\"" + wifiName + "\",\"" + wifiPass + "\"\r\n",10000))!=0);


  checkWlan();


}

void loop()
{

  if(esp8266.available()) // check if the esp is sending a message 
  {

    for(i=0; i<c_Length; i++){ 
      c[i] = c[i+1];
    }  
    c[c_Length] = esp8266.read();
    if(c[c_Length] < 32){ 
      c[c_Length] = ' ';
    }

    c_string = "";
    for(i=0; i<c_Length; i++){
      c_string += String(c[i+1]);
    }

    //Serial.print("c_string: ");
    //Serial.print(c_string);
    //Serial.println("   ");

    if( (StringContains(c_string, "+IPD") > -1) &&
      (StringContains(c_string, "GET") > -1) )
    {
      Serial.println(">+IPD");
      int connectionId = c[StringContains(c_string, "+IPD")+6] - 48;
      Serial.print("connectionId: ");
      Serial.println(connectionId);

      esp8266Flush();

      //if(connectionId==0){

      String webpage = "<h1>Hello! ";
      webpage += String(millis()/1000);
      webpage +="</h1>\r\n";

      String cipSend = "AT+CIPSEND=";
      cipSend += connectionId;
      cipSend += ",";
      cipSend +=webpage.length();
      cipSend +="\r\n";

      esp8266Flush();
      if (sendData(cipSend,5000)==0){
        sendData(webpage,5000);
      } 
      else if (sendData(cipSend,5000)==0){
        sendData(webpage,5000);
      }

      String closeCommand = "AT+CIPCLOSE="; 
      closeCommand+=connectionId; // append connection id
      closeCommand+="\r\n";

      sendData(closeCommand,5000);


    }
  }
}


int sendData(String command, const int timeout)
{
  Serial.println();
  Serial.print("command: "); 
  Serial.println(command);
  Serial.println("answer: "); 

  esp8266.print(command); // send the read character to the esp8266

  for(i = 0; i <= c_Length; i++){ 
    c[i] = ' ';
  }

  long int time = millis();
  while( (time + timeout) > millis())
  {
    while(esp8266.available())
    {
      for(i=0; i<c_Length; i++){ 
        c[i] = c[i+1];
      }  
      c[c_Length] = esp8266.read();
      if(c[c_Length] < 32){ 
        c[c_Length] = ' ';
      }

      c_string = "";
      for(i=0; i<c_Length; i++){
        c_string += String(c[i+1]);
      }

      //Serial.print("c: ");
      //Serial.print(c);
      //Serial.print(" ");
      //Serial.print("c_string: ");
      //Serial.print(c_string);
      //Serial.println("   ");

      if( StringContains(c_string, "busy inet...") > -1){ 
        Serial.print(">BUSY INET...");
        while((sendData("AT+RST\r\n",10000))!=0); // reset module
        checkWlan();      
      }

      if( (StringContains(c_string, "OK") > -1) ||
        (StringContains(c_string, "no change") > -1) ||
        (StringContains(c_string, "   >") > -1) 
        )
      {
        Serial.println(">OK");
        esp8266Flush();        
        return 0;
      }



      if( (StringContains(c_string, "ERROR") > -1) )
      {
        Serial.println(c_string);
        Serial.println(">ERROR");
        esp8266Flush();        
        return 4;
      }
    }  
  }


  Serial.println(c_string);
  Serial.println(">TIMEOUT");
  esp8266Flush();
  checkWlan();
  return 44;
}





int StringContains(String s, String search) {
  int max = s.length() - search.length();
  int lgsearch = search.length();

  for (int i = 0; i <= max; i++) {
    if (s.substring(i, i + lgsearch) == search) return i;
  }

  return -1;
}





void esp8266Flush(void)
{

  while(esp8266.available()) // clean buffer
  {
    esp8266.read();
  }
}




void checkWlan(void){
  while((sendData("AT+CWJAP?\r\n",3000))!=0){
    while((sendData("AT+CWJAP=\"" + wifiName + "\",\"" + wifiPass + "\"\r\n",10000))!=0){ 

      while((sendData("AT+RST\r\n",10000))!=0); // reset module
      while((sendData("AT+CWMODE=1\r\n",3000))!=0); //AP client mode

    }; //join to AP

    //    while((sendData("AT+CIFSR\r\n",3000))!=0); // get IP

  }; //check if it is joined to network
  while((sendData("AT+CIPMUX=1\r\n",3000))!=0); //multiple connection
  while((sendData("AT+CIPSERVER=1,80\r\n",3000))!=0); //create www server
  while((sendData("AT+CIPSTO=10\r\n",3000))!=0); //server timeout in sec 

  Serial.println("--- Ready! ---");

}



void wifiNamePassConversion(void){
  wifiName="";
  wifiPass="";

  for(i=0; i<=9; i++){
    if(wifiNamePass[0][i]==' ') break;
    wifiName = wifiName + wifiNamePass[0][i];
  }
  for(i=0; i<=9; i++){
    if(wifiNamePass[1][i]==' ') break;
    wifiPass = wifiPass + wifiNamePass[1][i];
  }



  for(i=0; i<=9; i++){
    lcd.setCursor(8+i, 1);
    lcd.print(wifiNamePass[0][i]);
    lcd.setCursor(8+i, 2);
    lcd.print(wifiNamePass[1][i]);


  }

  lcd.setCursor(0, 0);
  lcd.print(wifiName); lcd.print("-");
  lcd.print(wifiPass);lcd.print("-");


}






















































